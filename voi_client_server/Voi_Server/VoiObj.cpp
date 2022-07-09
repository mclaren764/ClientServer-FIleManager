#include "VoiObj.h"
#include "TypeVersionForCreate.h"

#include "ParserRmo.h"
#include "ParserBla.h"
#include "ParserKoir.h"
#include "ParserPoi.h"
#include "ParserRtr.h"
#include "ParserSpp.h"

#include "ModuleServerBase.h"
#include "ModuleServerMkcFuzz.h"
#include "ModuleServerBla.h"
#include "ModuleServerRmo.h"

#include "Plot.h"
#include "PlotComplan.h"
#include "SectBuf.h"
#include "TrackUnion.h"
#include "CommonConst.h"

#include "RangeAzList.h"
#include "RlkServer.h"
#include "Loger.h"
#include "ReadObj.h"
#include "TypesFunction.h"
#include "TrackBuf.h"
#include "SettingsBase.h"

#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QDataStream>
#include <FMObject.h>

#define sLogPrefix "VoiThread "

uint IDFromCommand(quint64 nID);
uint IdxFromCommand(quint64 nID);

extern uint n__SerialNumber;

QMap<uint, MyCellTypes> mProtToMyTypes;

inline MyCellTypes ProtToMyTypes(uint type) {
    if(mProtToMyTypes.contains(type)) return mProtToMyTypes.value(type);
    return eMyNoConvert;
}
//---------------------------------------------------------
VoiObj::VoiObj(uint nWkMode):QObject()
{
    setObjectName(tr("VoiObj"));
    TrackTrue::porogInit(2);
    m_rlk = nullptr;
    m_wkMode = nWkMode;
    m_elapsTimerNetSpeed = nullptr;

    ModuleBase::setWkMode(nWkMode);
    TrackUnion::n_CurTime = QDateTime::currentMSecsSinceEpoch();
    // для источников, отметки которых пишутся в один буфер,
    // сразу заведем буферы и инициализируем указатели
    m_bufKTBla = new SectBuf <PlotBla>(1200, 64); // буфер отметок НСУ БЛА
    if(!m_bufKTBla) {
        MY_LOG_ERR(tr("Не создался буфер отметок НСУ БЛА, "
                      "дальнейшая работа программы невозможна"));
        exit(EXIT_FAILURE);
    }
    ModuleServerBla::setBufBla(m_bufKTBla);

    m_bufKTKoir = new SectBuf <PlotKoir>(1200, 64); // Буфер отметок КОИР
    if(!m_bufKTKoir) {
        MY_LOG_ERR(tr("Не создался буфер отметок КОИР, "
                      "дальнейшая работа программы невозможна"));
        QCoreApplication::exit(EXIT_FAILURE);
    }
    m_FMObject = new FMObject;


    SectBuf <PlotRtr>* mbufRtrPlot = new SectBuf <PlotRtr>(500, TwoNsMax);
    SectBuf <DopInfoIri>* mbufRtrDopInfo = new SectBuf <DopInfoIri>(500, TwoNsMax);


    //connect(OTASK::GetNewTask(), &OMyEvent::NewTask, this, []() {
    //    OTASK::TaskStart();
    //}, Qt::QueuedConnection);

}
//---------------------------------------------------------
VoiObj::~VoiObj()
{
    //    delete[] po_TrackBuf;

    m_FMObject->deleteLater();
    delete m_bufKTBla;
    delete m_bufKTKoir;

    delete m_elapsTimerNetSpeed;

    QMapIterator <uint, RlkServer*> i(m_allRlk);
    while(i.hasNext()) {
        i.next();
        i.value()->deleteLater();
    }
    m_allRlk.clear();
    MY_LOG_INFO(__PRETTY_FUNCTION__);

}
//---------------------------------------------------------
void VoiObj::onStarted()
{
    MY_LOG_INFO(tr("Старт потока обработки"));
    TrackUnion::n_CurTime = QDateTime::currentMSecsSinceEpoch();

    TrackBuf& buf = TrackBuf::get();
    qRegisterMetaType<CommandSended>("CommandSended");

    // создаем "свой" Рлк
    m_rlk = new RlkServer(n__SerialNumber);
    if(!m_rlk) {
        MY_LOG_ERR(tr("Не создался объект \"комплекс\", дальнейшая работа программы невозможна."));
        QCoreApplication::exit(EXIT_FAILURE);
    }
    connectForOwnRlk();
    TrackUnion::m_allRlk = &m_allRlk;
    TrackUnion::m_rlk = m_rlk;

    RlkStatus& oSt = m_rlk->getStatus();
    m_rlk->setMobil();
    //m_rlk->setSerialNumber(n__SerialNumber);
    oSt.m_mode = m_wkMode;
    m_allRlk.insert(n__SerialNumber, m_rlk);
    if(m_allRlk.isEmpty()) {
        MY_LOG_ERR(tr("Не создался объект \"комплекс\", дальнейшая работа программы невозможна."));
        QCoreApplication::exit(EXIT_FAILURE);
    }

    addMkcFuzz();

    if(IsRdMode(m_wkMode)) { // чтение файла        

    } // чтение файла
    else { // боевая работа
        SettingsBase settings;
        bool ok;
        float period = settings.value(tr("Период комплекса, с"), 2.5f).toFloat(&ok);
        if(!ok || (period <= 0)) period = 2.5f;

        QTimer *pWriteNastrTimer = new QTimer;
        if(pWriteNastrTimer) {
            pWriteNastrTimer->setInterval(120000); // 2 минуты
            connect(pWriteNastrTimer, &QTimer::timeout, this, &VoiObj::onWriteNastrToKT);
            connect(this, &VoiObj::SigClose, pWriteNastrTimer, &QObject::deleteLater);
            pWriteNastrTimer->start();
        }
        else {
            MY_LOG_DEB(tr("Не создался таймер для периодической записи настроек в *.kt"));
        }
        m_elapsTimerNetSpeed = new QElapsedTimer;
        m_elapsTimerAm = new QElapsedTimer;
        QTimer *pTimeTimer = new QTimer;
        if(pTimeTimer) {
            pTimeTimer->setInterval(30);
            connect(this, &VoiObj::SigClose, pTimeTimer, &QObject::deleteLater);
            connect(pTimeTimer, &QTimer::timeout, this, [this, period]() {
                // установить время

                static uint amPrev = 0; // перыдущая выданная АМ
                static double azPrev = 0; // перыдущий выданный азимут
                static double TwoPiDouble = 2. * M_PI;
                static double timeToAz = TwoPiDouble / static_cast<double>(period) / 1000.;

                uint nsPrev = amPrev & NsMask;
                double azCur = azPrev + static_cast<double>(m_elapsTimerAm->elapsed()) * timeToAz;
                if(azCur > TwoPiDouble) azCur -= TwoPiDouble;
               // MY_LOG_DEB(tr("Timer: azNeed = %1, azCur = %2, delT = %3")
               //            .arg(azNeed).arg(azCur).arg(m_elapsTimerAm->elapsed()));
                uint nsCur = static_cast<uint>(azCur * AzToNs); // номер сектора, которому соответствует азимут
                uint delNs = (nsCur - nsPrev) & NsMask;
                if(delNs >= 1) { // АМ
                    amPrev += delNs;
                    azPrev = azCur;
                    qint64 curTime = QDateTime::currentMSecsSinceEpoch();

                    emit SigSinhroToKT(azCur, curTime); // запись синхросообщения=виртуальная СМ комплекса
                    TrackBuf::get().amReceived(azCur, curTime);
                 //   MY_LOG_DEB(tr("AmK: az = %1, time = %2")
                 //              .arg(azCur).arg(curTime));
                    m_elapsTimerAm->restart();
                }

                if(m_elapsTimerNetSpeed->hasExpired(1000)) { // каждую секунду
                    if(m_rlk) m_rlk->calcNetSpeed(float(m_elapsTimerNetSpeed->elapsed()) / 1000.f);
                    m_elapsTimerNetSpeed->restart();
                } // каждую секунду
                // задачи по собственному тактированию
                // секционирование общих буферов для отображения


            });

            // TODO
            //  один раз в обзор? берем самую приоритетную трассу ищем для нее ближайший
            //модуль оптики и камеру:
            // если нет свободных - выход
            // если не в зоне - пробуем следующую трассу
            // если взяли на сопровождение - пробуем следующую трассу

            connect(this, &VoiObj::SigClose, pTimeTimer, &QObject::deleteLater);
            pTimeTimer->start();
            m_elapsTimerNetSpeed->start();
            m_elapsTimerAm->start();
        }
        else {
            MY_LOG_DEB(tr("Не создался таймер для периодической записи настроек в *.kt"));
        }
    } // боевая работа    
}
//---------------------------------------------------------
void VoiObj::onFinished()
{
    MY_LOG_INFO(tr("Поток обработки остановлен"));
    emit SigClose();
    deleteLater();
}
//---------------------------------------------------------
void VoiObj::addMkcFuzz()
{
    AddModuleFromKT1 addModule;
    addModule.m_numbRlk = m_rlk->getSerialNumber();
    addModule.m_version = Version(2, 2);
    addModule.m_id = ID_SKC;
    addModule.m_idx = 250;
    addModule.m_packNumb = 0;
    addModule.m_addrIPv4 = 0;
    addModule.m_port = 0;
    Request req;
    req.m_idManuf = 0x1;
    req.m_serNumb = 0x1;
    req.m_verHardwareMaj = 0x1;
    req.m_verHardwareMin = 0x0;
    req.m_verSoftwareMaj = 0x1;
    req.m_verSoftwareMin = 0x0;
    req.m_isAsku = req.m_isInfo = 1;
    addModule.m_request = req;

    ModuleServerMkcFuzz* modMkcFuzz = new ModuleServerMkcFuzz(addModule);
    // добавим в карту модулей
    m_rlk->insertModule(addModule.m_id, addModule.m_idx, modMkcFuzz);
    MY_LOG_DEB(tr("В поток обработки добавлен модуль %1, номер в комплексе = %2")
               .arg(modMkcFuzz->getName()).arg(modMkcFuzz->getIdx()));

    TrackBuf::get().setMkcFuzz(modMkcFuzz);

    qRegisterMetaType<SendModuleDataToFromRmo>("SendModuleDataToFromRmo");
    //connect(modMkcFuzz, &ModuleBase::SigModulDataToFromRMO, p_Rlk, &OBaseRLK::SendModulDataToFromRMO);
}
//---------------------------------------------------------
void VoiObj::connectForOwnRlk()
{
    qRegisterMetaType<SendModuleDataToFromRmo>("SendModuleDataToFromRmo");
    connect(m_rlk, &RlkServer::moduleDataToFromRmoSended, this, &VoiObj::onSendModuleDataToFromRmo);
    qRegisterMetaType<SendRlkDataToRmo>("SendRlkDataToRmo");
    connect(m_rlk, &RlkServer::rlkDataToFromRmoSended, this, &VoiObj::onSendRlkDataToFromRmo);

    //qRegisterMetaType<OCoordModulToKT>("OCoordModulToKT");
    //connect(pRlk, &ORLKServer::SigCoordModulToKT, this, &VoiObj::SigCoordModulToKT);
}
//---------------------------------------------------------
void VoiObj::onModulePartDeleted(uint id, uint idx, ModulePart part) // удалить модуль или его часть
// от сетевого потока или объекта чтения файла
// этот сигнал всегда для своего РЛК
{    
    if(m_rlk) {
        if(part == ModulePartAll) {
            // удаление из буфера команд
            QMap<quint64, CommandToModule>::iterator i;
            QMap<quint64, CommandToModule>& rmCommBuf = m_rlk->getCommandBuf();
            for (i = rmCommBuf.begin(); i != rmCommBuf.end(); ) {
                if(IdxFromCommand(i.key()) == idx) i = rmCommBuf.erase(i);
                else ++i;
            }
        }
        if(part != ModulePartNo) {
            m_rlk->deleteModule(id, idx, part);
            // запись в *.kt
            DelModuleFromKT1 mod;
            mod.m_id = id; // идентификатор абонента
            mod.m_idx = idx; // номер модуля
            mod.m_isInfo = (part & ModulePartInfo) ? 1 : 0;  // удалить информационную часть
            mod.m_isAsku = (part & ModulePartAsku) ? 1 : 0;  // удалить аскушную часть
            mod.m_reserve = 0;
            emit writeDeleteModuleSended(mod);
        };
    }
    else {
        MY_LOG_ERR(tr("Получен запрос на удаление модуля %1_%2, "
                      "комплекс отсутствует в списке комплексов").arg(idToStr(id)).arg(idx));
    }
}
//---------------------------------------------------------
// запрос приходит от потока приема данных по сети или от объекта чтения,
// если подключается новый модуль,
// данный запрос всегда приходит для "своего" РЛК
void VoiObj::onNewModuleAdded(AddModuleFromKT1& addModule, ParserServerBase* parser)
// добавить новый модуль модуль
{
    if(!m_rlk) { // такого быть не должно
        MY_LOG_DEB(tr("Ошибка в программе при создании модуля %1_%2: "
                      "отсутствует объект РЛК")
                   .arg(idToStr(addModule.m_id)).arg(addModule.m_idx));
        return;
    }
    // создаем модуль
    PFunCreateSrvModule fun = TypeVersionForCreate::get().getFunCreateSrvModule(addModule.m_id);
    if(!fun) { // такого быть не должно
        MY_LOG_DEB(tr("Ошибка в программе при создании модуля %1_%2: "
                      "функция создания отсутствует")
                   .arg(idToStr(addModule.m_id)).arg(addModule.m_idx));
        return;
    }
    ModuleServerBase* module = fun(addModule);
    if(!module) { // такого быть не должно
        MY_LOG_DEB(tr("Ошибка в программе при создании модуля %1_%2: "
                      "объект модуля не создался")
                   .arg(idToStr(addModule.m_id)).arg(addModule.m_idx));
        return;
    }
    if(addModule.m_numbRlk == 0) addModule.m_numbRlk = m_rlk->getSerialNumber();
    emit writeAddModuleSended(addModule); // запись в *.kt добавления модуля

    qRegisterMetaType<ModulePositionFromKT1>("ModulePositionFromKT1");
    connect(module, &ModuleServerBase::modulePositionToKtSended, this, [this, addModule] (ModulePositionFromKT1 pos) {
        pos.m_id = MakeRlkIDIdx(addModule.m_numbRlk, pos.m_id);
        //emit SigCoordModulToKT(pos); // для потока обработки
    });
    m_rlk->addNewModule(addModule, parser, module);
    emit parserConnected(parser);
    // отправить координаты комплекса в модуль
    if(module->isNeedRlsCoord() && addModule.m_request.m_isInfo) {
        emit module->rlsPositionSended(m_rlk->rlkPos());
        emit module->rlsOrientSended(m_rlk->rlkOrient());
    }
    if( (addModule.m_id == ID_RMO && !ModuleBaseRmo::isFromFile(static_cast<uchar>(addModule.m_idx))) ) {
       ModuleServerRmo* rmo = dynamic_cast<ModuleServerRmo*>(module);
      if(rmo) {
          connect(rmo, &ModuleServerRmo::rmoStepReceived, this, &VoiObj::processEnded); // получено сообщение о порции данных
          connect(rmo, &ModuleServerRmo::rmoFMDataRecieved, this, &VoiObj::onRmoFMDataRecieved); //получено сообщение от файлового менеджера
          connect(this->m_FMObject, &FMObject::FMFileDataSended, this, [this, rmo](QByteArray arr){
              this->sendFMDataToRmo(FMDataHeader::GetFileList, arr, rmo);
          });
          connect(this->m_FMObject, &FMObject::FMCatalogDataSended, this, [this, rmo](QByteArray arr){
              this->sendFMDataToRmo(FMDataHeader::GetCatalog, arr, rmo);
          });
          connect(this->m_FMObject, &FMObject::sendFileSize, this, [this, rmo](QByteArray arr){
              this->sendFMDataToRmo(FMDataHeader::CopyFile, arr, rmo);
          });
          connect(this->m_FMObject, &FMObject::sendCopyPack, this, [this, rmo](QByteArray arr){
              this->sendFMDataToRmo(FMDataHeader::CopyFile, arr, rmo);
          });
          connect(this->m_FMObject, &FMObject::cancelCopy, this, [this, rmo](){
              QByteArray arr = nullptr;
              this->sendFMDataToRmo(FMDataHeader::CancelCopy, arr, rmo);
          });
          connect(this->m_FMObject, &FMObject::sendProcent, this, [this, rmo](QByteArray arr){
              this->sendFMDataToRmo(FMDataHeader::CopyFile, arr, rmo);
          });
          connect(this->m_FMObject, &FMObject::sendCopyStatus, this, [this, rmo](QByteArray arr){
              this->sendFMDataToRmo(FMDataHeader::CopyFile, arr, rmo);
          });
          connect(this->m_FMObject, &FMObject::readyToRecieveNext, this, [this, rmo](QByteArray arr){
              this->sendFMDataToRmo(FMDataHeader::CopyFile, arr, rmo);
          });
          connect(this->m_FMObject, &FMObject::finishCopy, this, [this, rmo](QByteArray arr){
              this->sendFMDataToRmo(FMDataHeader::FinishCopy, arr, rmo);
          });


      }
    }
}
//---------------------------------------------------------
void VoiObj::onModulePartAdded(AddModuleFromKT1 addModule, ParserServerBase* parser)    // добавить часть модуля
// добавить часть модуля от сетевого потока либо от потока чтения
// данный запрос всегда приходит для "своего" РЛК
{
    if(!m_rlk)  { // такого быть не должно
        MY_LOG_DEB(tr("Ошибка в программе при создании модуля %1_%2: "
                      "отсутствует объект РЛК")
                   .arg(idToStr(addModule.m_id)).arg(addModule.m_idx));
        return;
    }
    if(addModule.m_numbRlk == 0) addModule.m_numbRlk = m_rlk->getSerialNumber();
    emit writeAddModuleSended(addModule); // запись в *.kt добавления части модуля

    ModuleServerBase* module = m_rlk->addPartModule(addModule, parser);
    emit parserConnected(parser);
    // отправить координаты комплекса в модуль
    if( module && module->isNeedRlsCoord() && addModule.m_request.m_isInfo) {
        emit module->rlsPositionSended(m_rlk->rlkPos());
        emit module->rlsOrientSended(m_rlk->rlkOrient());
    }
    return;
}
//---------------------------------------------------------
void VoiObj::OnAllRLKForRMO(uint nIdx)
{
    // формируем всю информацию обо всех РЛК и отправляем на РМО (для только что подключившегося РМО)
    /*   if(p_Rlk) {
        ORLKServer *pRLKTec;
        const ORmoModulServer* pRmo = dynamic_cast<const ORmoModulServer*>(p_Rlk->GetRmoModule(nIdx));
        if(pRmo) {
            MY_LOG_INFO("Отправляем данные на рмо");
            SendRlkDataToRmo oSendData;
            QMapIterator<uint, ORLKServer*>  i(om_Rlk);
            while (i.hasNext()) {
                i.next();
                pRLKTec = i.value();
                oSendData.m_RlkSerNumb = pRLKTec->SerialNumber();
                oSendData.m_typeMess = RlkPositionType;
                emit pRmo->SigRLKData(oSendData, pRLKTec->MakeRLKCoordFoRMO()); // отправить сообщение о данных комплекса в парсер
                oSendData.m_typeMess = RlkOrientType;
                emit pRmo->SigRLKData(oSendData, pRLKTec->MakeRLKOrientFoRMO()); // отправить сообщение о данных комплекса в парсер
                oSendData.m_typeMess = RlkStatusType;
                emit pRmo->SigRLKData(oSendData, pRLKTec->MakeRLKStatusFoRMO(TrackUnion::n_CurTime)); // отправить сообщение о данных комплекса в парсер
                pRLKTec->MakeRLKModulesFoRMO(pRmo);
            }
        }
        else {
            MY_LOG_ERR(tr("Получен запрос информации обо всех РЛК для РМО %1, "
                          "в комплексе %2 РМО отсутствует в списке").arg(nIdx).arg(n__SerialNumber));
        }
    }
    else {
        MY_LOG_ERR(tr("Получен запрос информации обо всех РЛК, "
                      "комплекс %1 отсутствует в списке комплексов").arg(n__SerialNumber));
    }*/
}
//---------------------------------------------------------
//OTrack* VoiObj::GetTrack(uint nNc)
//{
//    if(nNc == 0 || nNc >= nMAXTrack) return nullptr;
//    return po_TrackBuf + nNc;
//}
//---------------------------------------------------------
void VoiObj::onWriteNastrToKT() // периодическая запись настроек и всех подключенных модулей в *.kt
{

}
//---------------------------------------------------------
void VoiObj::sendStepToRmo()
// отправка на РМО сообщения, что порция данных прочитана
{
    if(!m_rlk) return;
    ModuleServerRmo* rmo;
    QMapIterator<uint, ModuleServerBase*> i(m_rlk->getModules(ID_RMO));
    bool isSend = false;
    while (i.hasNext()) {
        i.next();
        rmo = dynamic_cast<ModuleServerRmo*>(i.value());
        if(rmo) {
            //MY_LOG_DEB(tr("onSendModuleDataToFromRmo  serN=%1").arg(header.m_rlkSerNumb));
            emit rmo->toRmoStepSended (); // отправить сообщение о данных модуля в парсер
            //MY_LOG_DEB(tr("Отправка данных о модуле на клиента: тип = %1")
            //           .arg(header.m_typeMess));
            isSend = true;
        }
    }
    if(!isSend) emit processEnded();
}
//---------------------------------------------------------
void VoiObj::onSendModuleDataToFromRmo (SendModuleDataToFromRmo header, QByteArray data)
// отправка информации о модуле всем подключенным РМО
{
    if(!m_rlk) return;
    ModuleServerRmo* rmo;
    QMapIterator<uint, ModuleServerBase*> i(m_rlk->getModules(ID_RMO));
    while (i.hasNext()) {
        i.next();
        rmo = dynamic_cast<ModuleServerRmo*>(i.value());
        if(rmo) {
            //MY_LOG_DEB(tr("onSendModuleDataToFromRmo  serN=%1").arg(header.m_rlkSerNumb));
            emit rmo->toRmoModuleDataSended (header, data); // отправить сообщение о данных модуля в парсер
            //MY_LOG_DEB(tr("Отправка данных о модуле на клиента: тип = %1")
            //           .arg(header.m_typeMess));
        }
    }
}
//---------------------------------------------------------
void VoiObj::onSendRlkDataToFromRmo(SendRlkDataToRmo header, QByteArray data)
// отправка информации о модуле всем подключенным РМО
{
    if(!m_rlk) return;
    ModuleServerRmo* rmo;
    QMapIterator<uint, ModuleServerBase*> i(m_rlk->getModules(ID_RMO));
    while (i.hasNext()) {
        i.next();
        rmo = dynamic_cast<ModuleServerRmo*>(i.value());
        if(rmo) {
            MY_LOG_DEB(tr("onSendRlkDataToFromRmo  serN=%1").arg(header.m_rlkSerNumb));
            emit rmo->toRmoRlkDataSended (header, data); // отправить сообщение о данных модуля в парсер
            MY_LOG_DEB(tr("Отправка данных о Рлк на клиента: тип = %1")
                       .arg(header.m_typeMess));
        }
    }
}
//----------------------------------------------------------------------------
void VoiObj::onRmoFMDataRecieved(QByteArray data)
{
//    int size = data.size();
//    if(size < sizeof(FMDataHeader)) {
//        return;
//    }
    const FMDataHeader* header = reinterpret_cast<const FMDataHeader*>(data.data());
    QByteArray mess = data.right(data.size()-sizeof(FMDataHeader));
    m_FMObject->receiveFMCommandFromRmo(mess, header->m_type);
}
//---------------------------------------------------------------------------
void VoiObj::sendFMDataToRmo(uint typeMes, QByteArray data,
                               const ModuleServerRmo* rmo)
{
    FMDataHeader header(typeMes);
    data.prepend(reinterpret_cast<const char*>(&header), sizeof(header));
    if(rmo) {
        emit rmo->toRmoFMDataSended(data); // отправить сообщение о зонах ответственности в парсер
        return;
    }

//    QMapIterator<uchar, ModuleServerBase*> i(getModulesTypeMap(ID_RMO));
//    while (i.hasNext()) {
//        i.next();
//        rmo = dynamic_cast<ModuleServerRmo*>(i.value());
//        if(rmo) {
//            emit rmo->toRmoFMDataSended (data); // отправить сообщение о зонах ответственности в парсер
//            //MY_LOG_DEB(tr("Отправка данных о трассе на клиента: тип = %1")
//            //           .arg(header.m_typeMess));
//        }
//    }
}
//----------------------------------------------------------------
/*
int OVoiThread::FindKTForMkcBR(int nKTIdx, int& nNc)
{
   if(nNc < 1 || nNc > NTR) return -1;
   return aoTrBuf[nNc].o_MkcSled.FindKT(nKTIdx);
}

//----------------------------------------------------------------
int OVoiThread::FindKTForMkcRD(int nKTIdx, int& nNc)
{
    CELL *pTr;
    for(int i = 1; i < NTR; i++) {
      pTr = aoTrBuf + i;
      if(pTr->e_TrState < TR_P) continue;
      int nFind = pTr->o_MkcSled.FindKT(nKTIdx);
      if(nFind > -1) {
        nNc = i;
        return nFind;
        }
      }
    return -1;
}

//----------------------------------------------------------------
void OVoiThread::SetNewCellType(OMkcTypeCell oType) // прием типа отметки от распознавалки
{
   uint nT = ProtToMyTypes(oType.n_TypeCell);
   uint nTR = oType.n_TypeCell;

   if(nT > 0 && nT <= eProtMaxType && oType.f_Probab[nTR - 1] < OMkcModul::o_Nastr.f_ProbabMkc) nT = eMyUnknown;

   int nNc = oType.n_IdxCell;
   int nKTInSled = (this->*p_FunFindKTForMkc)(oType.n_IdxKT, nNc);
   if(nKTInSled < 0) return;
   oType.n_IdxCell = nNc;

   // пробуем обновить последнюю отметку
   CELL *pTr = aoTrBuf + nNc;
   KT_RLS *pKT = pTr->o_CombKT.P_PRL();
   if(pKT && pKT->r_KTNumb == oType.n_IdxKT) {
     float fOSSHMax = -30.f;
     for(int i = 0; i < e_MaxBeams; i++) {
       if(pKT->f_PowerRF[i] > fOSSHMax) fOSSHMax = pKT->f_PowerRF[i];
       }
     if(fOSSHMax < OMkcModul::o_Nastr.f_OSSHPorog) nT = eMyUnknown;
     pKT->o_Type = oType;
     pKT->n_TypeMkc = nT;
     }
   // отправляем для следа
   emit SigNewType(oType);
   // записываем в след МКЦ и анализируем тип трассы
   int nType = pTr->o_MkcSled.AddType(nKTInSled, nT);
   if(nType < 0) return;
   pTr->n_KFromMCntAll++;
   pTr->n_KFromMCntType[nType]++;

    int nMaxIdx = -1, nMaxCnt = -1;
    for(int i = 0; i < eMyMaxType; i++) {
      if(pTr->n_KFromMCntType[i] > nMaxCnt)   {
        nMaxCnt = pTr->n_KFromMCntType[i];
        nMaxIdx = i;
        }
      }
    if(nMaxCnt == pTr->n_KFromMCntType[nType]) pTr->n_TrTypeMKC = nType;
    else pTr->n_TrTypeMKC = nMaxIdx;
    if(pTr->IsOnlyMkcType()) pTr->n_TrType = pTr->n_TrTypeMKC;
    return;
}*/
// чтение сообщений из файла *.kt (в старом варианте, в новом варианте
// часть настроек уйшли в разные типы модулей)
//---------------------------------------------------------
// сообщение оо всех подключенных модулях
// считать модуль из файла *.kt из сообщения обо всех подключенных модулях
// старый вариант
void VoiObj::onModuleFromKt(QByteArray oBA, Version verVoi, uint id, uint idx)
{
    ModuleServerBase* module = m_rlk->getModule(id, idx);
    if(module) {
        QDataStream dataStream(oBA);
        module->readFromKt(dataStream, verVoi); // такой модуль добавился
        emit moduleFromAllModulesReaded(oBA.remove(0, dataStream.device()->pos()));
    }
    else {
        MY_LOG_ERR(tr("Сопоставление всех подключенных модулей: ID = %1 (0x%2), idx = %3 не "
                      "добавился в карту модулей").arg(idToStr(id)).
                   arg(id, 0, 16).arg(idx));
        emit moduleFromAllModulesReaded(QByteArray());
        return;
    }
}
//---------------------------------------------------------
// считать модуль из файла *.kt из сообщения обо всех подключенных модулях в формате JSon
// текущий вариант
void VoiObj::onModuleFromKtNew(QJsonObject object, Version verVoi, uint id, uint idx)
// все подключенные модули из файла
{
    ModuleServerBase* module = m_rlk->getModule(id, idx);
    if(module) module->readFromKt(object, verVoi); // такой модуль добавился
    else {
        MY_LOG_ERR(tr("Сопоставление всех подключенных модулей: ID = %1 (0x%2), idx = %3 не "
                      "добавился в карту модулей").arg(idToStr(id)).
                   arg(id, 0, 16).arg(idx));
        return;
    }
}
//----------------------------------------------------------------
void VoiObj::onVoiKtConfigReceived(QByteArray ba, ReadedData rd)
// настроки ВОИ из файла кт в старом варианте
{
    QDataStream ds(ba);
    qint64 time;
    ds>>time;
    // параметры РЛП
    int n;
    ds>>n;
    if(n <= 0 || n > 1) return;
    ds>>n;
    // Идентификация РЛП
    ds>>n>>n>>n;
    // Гео координаты точки стояния станции
    float lat, lon, alt;
    ds>>lat>>lon;
    float f;
    ds>>n>>alt>>f>>f;
    ds>>n>>f>>f;
    // работа с ВИП
    ds>>n;

    QString str;
    ds>>str;

    if(rd.m_isPositionRlk) { // если нужно считать позицию комплекса применим ее
        m_rlk->setPosition(lat * GradToRad, lon * GradToRad, alt); //
    }
    // настройки обработки трасс (все типа float):
    // максимальное ускорение
    // минимальная скорость
    // максимальная скорость
    // максимальная выдаваемая дальность
    QString aMax, vMin, vMax, dMax;
    ds>>aMax>>vMin>>vMax>>dMax;
    if(rd.m_isTrackProc) {
        bool ok;
        float aMaxValue = aMax.toFloat(&ok);
        if(ok && (aMaxValue > 0.f) && (aMaxValue < 80.f) ) {
            float vMaxValue = vMax.toFloat(&ok);
            if(ok && (vMaxValue > 0.f) && (vMaxValue < 2000.f) ) {
                m_rlk->readAVMaxPoiFromKtOld(aMaxValue, vMaxValue);
            }
        }
    }
    // настройки собственной карты помех
    //oDS>>n>>oSSB.f_PorogKar>>oSSB.f_Smooth;
    //for(int i = 0; i < e_MaxBeams; i++) oDS>>oSSB.n_IspKar[i];
    ds>>n>>f>>f;
    ds>>n>>n>>n>>n;
    if(rd.m_isConfigKartaPomeh) {
        //oKartaPomeh.SetPorog(oSSB.f_PorogKar);
        //oKartaPomeh.setSmooth(oSSB.f_Smooth);
        //oKartaPomeh.SetMapOn(oSSB.n_IspKar);
    }
    if(rd.m_isFiToH) m_rlk->readFiToHPoiFromKtOld(ba.remove(0, ds.device()->pos()));
    return;
}
//----------------------------------------------------------------
void VoiObj::onVoiKtModuleNastrReceived(QByteArray ba, ReadedData rd)
// настроки модулей из файла кт в старом варианте
{
    // считывание всех данных из буфера
    QJsonDocument jsonDoc = QJsonDocument::fromBinaryData(ba);
    QJsonObject object = jsonDoc.object();
    if (!object.contains("ModulType")) return;
    int id = static_cast<int>(object["ModulType"].toInt());
    switch (id) {
    case ID_SKC:
        //ReadMkcNastr(oJsonObject, OMkcModul::o_Nastr);
        break;
    case ID_PEL:
        //ReadSppNastr(oJsonObject, OSppModul::o_Nastr);
        break;
    case ID_OTS:
        //ReadKoirNastr(oJsonObject, OKoirModul::o_Nastr);
        break;
    case ID_POI:
        // настройки фильтрации местников, метео и выдача запросов КН
        if(rd.m_isMest) m_rlk->readMestConfPoiFromKtOld(object);
        if(rd.m_isMeteo) m_rlk->readMeteoConfPoiFromKtOld(object);
        if(rd.m_isAutoKn) m_rlk->readAutoKnConfPoiFromKtOld(object);
        break;
    default: return;
    }
}
//----------------------------------------------------------------
void VoiObj::onVoiKtCoordModuleReceived(ModulePositionFromKT position)
// координаты модуля в старом варианте
{
    m_rlk->setModulePositionFromKt(ModulePositionFromKT1(position));
}
//----------------------------------------------------------------
void VoiObj::onVoiKtCoordModule1Received(ModulePositionFromKT1 position)
// координаты модуля в текущем варианте
{
    uint serN = rlkFromRlkIdIdx(position.m_id);
    if(m_allRlk.contains(serN)) m_allRlk.value(serN)->setModulePositionFromKt(position);
}
//----------------------------------------------------------------
void VoiObj::onfileKtCurTimeReaded(qint64 time)
// чтение синхросообщения из файла в старом формате
{
    static uint amPrev = 0; // предыдущая выданная АМ
    static float azPrev = 0.f; // предыдущий выданный азимут
    static float timeToAz = TwoPi / 2500.; // в старом формате период был всегда 2.5 секунды
    static qint64 timePrev = -1; // предыдущее пришедшее время


    if(timePrev < 0) {
        timePrev = time;
        sendStepToRmo();
        return;
    }

    qint64 difT = time - timePrev;
    qint64 delta = std::abs(difT - 2500 / 32);
    if(delta > 150) {
        timePrev = -1;
        sendStepToRmo();
        return;
    }
    if(delta > 20) difT = 2500 / 32;
    while(timePrev + difT < time) {
        uint nsPrev = amPrev & NsMask;
        float azCur = SumAz(azPrev, static_cast<double>(difT) * timeToAz);
        uint nsCur = static_cast<uint>(std::round(azCur * AzToNs)); // номер сектора, которому соответствует азимут
        uint delNs = (nsCur - nsPrev) & NsMask;
        if(delNs >= 1) { // АМ
            amPrev += delNs;
             TrackBuf::get().amReceived(azCur, timePrev + difT);
        }
        azPrev = azCur;
        timePrev += difT;
    }
    sendStepToRmo();
 }
// настройки фаззификатора старый вариант, в текущем варианте
// настройки пишутся прям в настройках модуля МКЦ
//----------------------------------------------------------------
/*QByteArray OVoiThread::RecvFuzzPoints(QByteArray oByteArray)
{
  QJsonDocument oJsonDocument = QJsonDocument::fromBinaryData(oByteArray);
  QJsonObject oJsonObject = oJsonDocument.object();
  if (!(oJsonObject.contains("I") && oJsonObject.contains("J"))) return QByteArray();
  int i = oJsonObject.take("I").toInt(), j = oJsonObject.take("J").toInt();
  QMap<float, float>& mapPoints = OFuzzificator::ao_Fuzzif[i][j].m_Points;
  mapPoints.clear();
  for (const auto& crStrKey : oJsonObject.keys()) {
    mapPoints[crStrKey.toFloat()] = static_cast<float>(oJsonObject[crStrKey].toDouble());
    }
  oStartParam.FuzzToFilePoints(i, j); // запишется только в БР
  return oByteArray;
}
//----------------------------------------------------------------
QByteArray OVoiThread::RecvFuzzWeight(QByteArray oByteArray)
{
  QJsonDocument oJsonDocument = QJsonDocument::fromBinaryData(oByteArray);
  QJsonObject oJsonObject = oJsonDocument.object();
  if (!(oJsonObject.contains("I") && oJsonObject.contains("J"))) return QByteArray();
  int i = oJsonObject.take("I").toInt(), j = oJsonObject.take("J").toInt();
  OFuzzificator::ao_Fuzzif[i][j].f_Weight = static_cast<float>(oJsonObject["Weight"].toDouble());
  oStartParam.FuzzToFileWeight(i, j); // запишется только в БР
  return oByteArray;
}
//----------------------------------------------------------------
void OVoiThread::OnConfigToKT() // периодическая запись настроек в файл *.kt
{
    QByteArray oBA;
    QDataStream oDS(&oBA, QIODevice::WriteOnly);
    oDS<<QDateTime::currentMSecsSinceEpoch();
    oRLP_Param.WriteConfigToBuf(oDS);
    oDS<<oStartParam.ReadFromFileVip();
    OSaveItem* pItem = oS_AMaxVMin.ItemList();
    while(pItem) { // Выполняем, пока не закончатся элементы в секции
      oDS<<QString(pItem->ValueStr());
      pItem = pItem->NextItem();
      }
    //oDS<<nIsFiltrMeteo<<oSSB.f_PorogKar<<oSSB.f_Smooth;
    oDS<<int(0)<<oSSB.f_PorogKar<<oSSB.f_Smooth;
    for(int i = 0; i < e_MaxBeams; i++) oDS<<oSSB.n_IspKar[i];
    //oDS<<nIs
    //oBA.append(oUgolMesta.WriteToBuf());
    oUgolMesta.WriteToBuf(oDS);
    emit SendConfigToKT(oBA);

    emit SigModulNastr(MakeMkcNastr(OMkcModul::o_Nastr)); // сформируем настройки для МКЦ и отправим из ВОИ на ИКО и в *.kt
    emit SigModulNastr(MakeSppNastr(OSppModul::o_Nastr)); // сформируем настройки для СПП и отправим из ВОИ на ИКО и в *.kt
    emit SigModulNastr(MakeKoirNastr(OKoirModul::o_Nastr)); // сформируем настройки для КОИР и отправим из ВОИ на ИКО и в *.kt
    emit SigModulNastr(MakeBlaNastr(OBlaModul::o_Nastr)); // сформируем настройки для БЛА и отправим из ВОИ на ИКО и в *.kt
    emit SigModulNastr(MakePoiNastr(o_NastrPoi)); // сформируем настройки для ПОИ и отправим из ВОИ на ИКО и в *.kt
    emit SigTrOutNastr(MakeTrOutNastr(o_NastrTrOut)); // сформируем настройки для выдачи трасс и отправим из ВОИ на ИКО и в *.kt

    for (int i = 0; i < OFuzzificator::n_TypeCnt; ++i) {
    for (int j = 0; j < OFuzzificator::n_PriznCnt; ++j) {
      emit SigFuzzPointsKT(MakeFuzzPoints(i, j, OFuzzificator::ao_Fuzzif[i][j].m_Points));
      emit SigFuzzWeightKT(MakeFuzzWeight(i, j, OFuzzificator::ao_Fuzzif[i][j].f_Weight));
      }
    }
}

//----------------------------------------------------------------
void OVoiThread::RevcTrOutNastr(QByteArray oBA) // при чтении из *.kt и при приеме из графики
{
    // считывание всех данных из буфера
    QJsonDocument oJsonDocument = QJsonDocument::fromBinaryData(oBA);
    QJsonObject oJsonObject = oJsonDocument.object();
    ReadTrOutNastr(oJsonObject, o_NastrTrOut);
    oStartParam.TrOutNastrToFile(); // запишется только в БР
    emit SigTrOutNastr(oBA); // может будем посылать только при приеме от парсера FIXME
}
//----------------------------------------------------------------
void OVoiThread::RecvFuzzPointsKT(QByteArray oByteArray) // приходит из гуишки
{
  auto oBA = RecvFuzzPoints(oByteArray);
  if (!oBA.isEmpty()) emit SigFuzzPointsKT(oByteArray); // для записи в *.kt
}
//----------------------------------------------------------------
void OVoiThread::RecvFuzzWeightKT(QByteArray oByteArray) // приходит из гуишки
{
  auto oBA = RecvFuzzWeight(oByteArray);
  if (!oBA.isEmpty()) emit SigFuzzWeightKT(oByteArray); // для записи в *.kt
}
//----------------------------------------------------------------
void OVoiThread::RecvFuzzPointsGUI(QByteArray oByteArray) // приходит из файла *.kt
{
  auto oBA = RecvFuzzPoints(oByteArray);
  if (!oBA.isEmpty()) emit SigFuzzPointsGUI(oByteArray); // для гуи
}
//----------------------------------------------------------------
void OVoiThread::RecvFuzzWeightGUI(QByteArray oByteArray) // приходит из файла *.kt
{
  auto oBA = RecvFuzzWeight(oByteArray);
  if (!oBA.isEmpty()) emit SigFuzzWeightGUI(oByteArray); // для гуи
}
*/
