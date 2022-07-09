#ifndef VOITHREAD_H
#define VOITHREAD_H

#include <QObject>
#include <QMap>

#include "ParserTypes.h"
#include "RmoTypes.h"
#include "VoiKtTypes.h"

#include "Enums.h"
#include "KoirTypes.h"
#include "VoiTypes.h"

template <class T> class RangeAzList;

class TrackUnion;
class ParserServerBase;
//template <class T> class OSectBufAsync;
template <class T> class SectBuf;
template <class T> class RingBuf;
class PlotBla;
class PlotRls;
class PlotKoir;
class PlotPriznKoir;
class PlotComplan;
class ZasPlotRls;
class PoiNoiseMap;
class TrSrcStrobData;
class FMObject;

class OSettings;
class RlkServer;
class ReadObject;
class ModuleServerBase;
class ModuleServerRmo;
class ORmoModulServer;
class QElapsedTimer;
//struct ReadedData;

class VoiObj:public QObject
{
    Q_OBJECT

    uint m_wkMode;

    RlkServer *m_rlk;
    QMap<uint, RlkServer*> m_allRlk; // все комплексы, ключ - серийный номер (= 3 байта)
    // если серийный номер совпадает со своим, то это текущий комплекс,
    // остальные подключены сервер-сервер как ведомые
    QElapsedTimer *m_elapsTimerNetSpeed;
    QElapsedTimer *m_elapsTimerAm;


protected:


//    bool event(QEvent *event) override;
public:
    VoiObj(uint wkMode);
    ~VoiObj();

    SectBuf <PlotBla>* m_bufKTBla; // буфер отметок НСУ БЛА
    SectBuf <PlotKoir>* m_bufKTKoir; // Буфер отметок КОИР
    SectBuf <PlotPriznKoir>* m_bufKTPriznKoir; // Буфер признаков отметок КОИР
    SectBuf <KoirImage>* m_bufImageKoir; // Буфер изображений отметок КОИР

    // для Пои
    SectBuf <PlotRls>* m_bufPlotPoi;        // Буфер отметок
    RingBuf <PlotComplan>* m_bufPlotComplanPoi; // буфер отметок квадратур для расчета спектров
    SectBuf <ZasPlotRls>* m_bufZasPoi;      // буфер засечек
    SectBuf <PoiNoiseMap>* m_bufNoiseMapPoi; // буфер карты помех
    SectBuf <TrSrcStrobData>* m_bufTrSrcStrobDataPoi; // исходные данные в стробе

    FMObject *m_FMObject;

    void connectForOwnRlk();
    void addMkcFuzz();
    void moduleCoordFromKt();
    void sendStepToRmo();

public slots:

    void onStarted();   // обработка сигнала о запуске потока
    void onFinished();  // обработка сигнала о завершении потока
    // от таймеров
    void onWriteNastrToKT(); // периодическая запись настроек и всех пдключенных модулей в *.kt

    void onModulePartDeleted(uint, uint, ModulePart); // удалить модуль от сетевого потока
    void onModulePartAdded(AddModuleFromKT1, ParserServerBase*);    // добавить часть модуля
    void onNewModuleAdded(AddModuleFromKT1&, ParserServerBase*);    // добавить новый модуль модуль
                               // или потока чтения
    void onVoiKtCoordModuleReceived(ModulePositionFromKT); // координаты модуля в старом варианте
    void onVoiKtCoordModule1Received(ModulePositionFromKT1);// координаты модуля в текущем варианте
    void onVoiKtConfigReceived(QByteArray ba, ReadedData); // настроки ВОИ из файла кт в старом варианте
    void onVoiKtModuleNastrReceived(QByteArray ba, ReadedData); // настроки модулей из файла кт в старом варианте
    void onModuleFromKt(QByteArray oBA, Version verVoi, uint id, uint idx); // все подключенные модули из файла
    void onModuleFromKtNew(QJsonObject, Version verVoi, uint id, uint idx); // все подключенные модули из файла
    void onfileKtCurTimeReaded(qint64 time); // чтение синхросообщения из файла
    void OnAllRLKForRMO(uint nIdx);

    // отправка на отображение всем подключенным клиентам
    void onSendModuleDataToFromRmo (SendModuleDataToFromRmo header, QByteArray data);
    void onSendRlkDataToFromRmo (SendRlkDataToRmo header, QByteArray data);
    void sendFMDataToRmo(uint typeMes, QByteArray data,
                         const ModuleServerRmo* rmo = nullptr);
    void onRmoFMDataRecieved(QByteArray);

signals:
    void SigTestAns();
    void SigClose();
    void SigSinhroToKT(float az, qint64 time);
    void SigRequestAnswer(uint nSockId, uint nErr, ParserServerBase* pParser, Request oReq); // ответ на запрос о подключении модуля
    void SigRequestAnswerRead(ParserServerBase* pParser, Request oReq, bool bContinue); // ответ на запрос о подключении модуля
    void SigNewCommand(quint64 nID, uint nIDCom, qint64 nTime, QString sDescr); // для сетевого потока
    // он должен записать отправленную команду в *.kt и отправить на отображение в клиент
    //void SigCoordModulToKT(OCoordModulToKT);  // для записи в файл *.kt

    void moduleFromAllModulesReaded(QByteArray);
    void parserConnected(ParserServerBase*);
    void processEnded() const;
    // для записи в *.kt
    void writeDeleteModuleSended(DelModuleFromKT1);
    void writeAddModuleSended(AddModuleFromKT1);
    void SigWriteAllModules(QByteArray);
    void SigCoordModulToKT(ModulePositionFromKT);
    void FMDataSended(QByteArray);

};

/*
//-------------------------------------------------------------------------------------------------
class OVoiThread :public QObject
{
    Q_OBJECT
   int (OVoiThread::*p_FunFindKTForMkc)(int nKTIdx, int& nNc);
public:
    static OPoiVoiNastr o_NastrPoi;
    static OTrOutNastr o_NastrTrOut; // настройки для выдачи целей внешним потребителям
    QMap<int, OModulInVoi*> o_MapModul;  // все подключенные модули
    QMap<int, int> o_MapBlaOut;
    QMap<quint16, OBaseZoneRespons*> o_ZoneMap; // список зон отвественности
    QList<OCellForZone> o_CellList; // список целей, находящихся в зоне или приближающихся к ней

    OVoiThread(QThread *pThr);
    ~OVoiThread();
    OModulInVoi* GetModul(int nIdx, int nID);
    int GetKey(int nIdx, int nID) { return ((nIdx << 24) | nID); }
    int FindKTForMkcBR(int nKTIdx, int& nNc);
    int FindKTForMkcRD(int nKTIdx, int& nNc);
    void SetFunFindKTForMkc(bool bIsBr) {
      if(bIsBr) p_FunFindKTForMkc = &OVoiThread::FindKTForMkcBR;
      else p_FunFindKTForMkc = &OVoiThread::FindKTForMkcRD;
    }
    float DeleteCellFromListZone(CELL* pTr);
    void WriteCellToListZone(OCellForZone* pZone);
    void SendAvtomTrackToKoir(int nNc = -1);

 signals:
    void SendStrobToPoi2A(OToPoi2aStrob);
    void SendStrobToKoir(OToKoirStrob, int);
    void SendToMkc(QByteArray);
    void SendStrobToSpp(OToSppStrob);
    void SendCellToBla(int, int, OToBlaCell);
    void DeleteTrackFromKoirF(QVector<quint8>);
    void DeleteTrackFromKoirSopr(QVector<quint16>); // удаление из модулей трассы при сбросе трассы
    void DeleteTrackFromSpp(QVector<quint8>); // удаление из модулей трассы при сбросе трассы
    void DeleteTrackFromBla(QVector<quint8>, int nNc);

    // для сетевого потока
    void SigKoirAvtomSopr(OToKoirSopr, bool); // вкл./выкл. автоматическое сопровождение трассы оптикой

    void SigNewType(OMkcTypeCell); // для ИКО для обновления следа
    void SigNewParamRlp(); // при чтении параметров из файла *.kt смена максимальной дальности
    // Для записи в файл *.kt
    void SendKartaPomToKT(QByteArray);
    void SendConfigToKT(QByteArray);
    void SendRLISyncToKT(ORLISyncToKT);

    void CloseSignal();
    void SigTrackToKoir(OToKoirSopr); // обновление координат при сопровождениии трассы оптикой
    void SigREBCoordFromTrack(int, float, float, float, float, float); // обновление координат для подавления
                  // по данной трассе
    void SigTrackOut(OTrackOut); // выдача сообщени о трассе всем заинтересованным модулям

    void SigModulNastr(QByteArray); // настройки для обработки для каждого типа модулей в формате Json
    void SigTrOutNastr(QByteArray); // настройки для выдачи трасс на внешних потребителей в формате Json
       // сигнал для отображения и для записи в *.kt
       // такой же сигнал будет приходить от отображалки  и от парсера
    void SigUpdateTrackList(QList<OCellForZone>); // обновление списка "опасных" целей
    void SigUpdateTrackSppList(QList<OIRICell>); // обновление списка ИРИ
    void SigAddZone(float fX, float fY, float fR); // добавить зону для отображения
    void SigDopInfoSpp(ODopInfoBla); // пересылка доп. информации о БЛА от пеленгатора
    void signalAddZone(quint16, OBaseZoneRespons*);
    void SigSistErr(int, int, float, float, float);
    void SigToModulSZI(QVector<OSZI>, bool);
    void SigToModulNoRad(bool);

    void SigAddPointToHToFi(float, float, float, float, float, float, unsigned short); // добавление точки для сбора статистики
    // по калибровке разности фаз от угла места
    void SigTecFiToH(OFiToH*); // добавление точки для сбора статистики
    void SigAddTecPoint(float, float, float, float); // добавление точки для сбора статистики
public slots:
    void OnStarted();   // обработка сигнала о запуске потока
    void OnFinished();  // обработка сигнала о завершении потока

    void OnConfigToKT();
    void RecvConfigFromKT(QByteArray);
    void RecvRLISyncFromKT(ORLISyncFromKT);

    void OnDeleteModul(int nID, int nIdx);
    void OnAddModul(int nID, int nIdx, OModulInVoi* pModul);
    void ModToTrackREB(int nIdx, int nNc, bool bAdd); // добавить/удалить номер модуля в трассу для подавления
    void KoirToTrackSopr(int nIdx, int nNc, bool bAdd); // добавить/удалить номер модуля в трассу для сопровождения оптикой
    void ModToTrackBla(int nIdx, int nNc, bool bAdd); // добавить/удалить номер модуля в трассу для Анти-БЛА

    //void SetNewCellType(OMkcTypeCell, int); // прием типа отметки от распознавалки
    void SetNewCellType(OMkcTypeCell); // прием типа отметки от распознавалки

    void SetBirdFromOper(int nNc); // прием задать тип цели птица от оператора
    void SetUVAFromOper(int nNc); // прием задать тип цели БЛА от оператора
    void SetTrOutFromOper(int nNc); // прием задать тип цели птица от оператора

    void RevcModulNastr(QByteArray); // прием настроек для каждого типа модулей в формате Json от ИКО либо от парсера
    void RevcTrOutNastr(QByteArray); // прием настроек для выдачи трасс внешним потребителям в формате Json от ИКО либо от парсера
    void RecvSM(ORecvSM oSm); // прием секторной метки ПОИ от парсера
    void RecvPoiSZI(QVector<OSZI>); // прием СЗИ от ПОИ
    void slotAddZone(quint16, OFigZone*);
    void slotChangeZone(quint16, OFigZone*);
    void slotDeleteZone(quint16);
    void OnNewPopr(int nID, int nIdx, float fDelAz, float fDelD, float fDelEps); // для потока ВОИ
    void SetPoiEpsAnt(int nIdx, float fEps);
   };

*/

#endif // VOITHREAD_H
