#include "FileManager.h"
#include "CopyDialog.h"
#include "HalfDlg.h"
#include "RecreateDlg.h"
#include "QVBoxLayout"
#include "QHBoxLayout"
#include <QDebug>
#include <QSettings>
#include <RmoTypes.h>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <RmoTypes.h>


const int m_fmWidth = 800;      // ширина окна файлового менеджера
const int m_fmHeight = 500;     // высота окна файлового менеджера
const int m_btnHeight = 18;     // высота кнопки

//============================================================================================

FileManager::FileManager(QWidget* parent)
        :QWidget(parent, Qt::Window)
{
    setMinimumSize(m_fmWidth + 15, m_fmHeight);
    setWindowTitle("Файловый менеджер");
    m_splitter = new QSplitter(this);
    m_dialog1 = new HalfDlg(m_splitter);
    m_dialog2 = new HalfDlg(m_splitter);
    m_dialog1->setOtherDlg(m_dialog2);
    m_dialog2->setOtherDlg(m_dialog1);
    m_dialog1->setClient();
    m_dialog2->setClient();
    m_viewButton = new QPushButton(tr("F3 Просмотр"), this);
    m_viewButton->setShortcut(QKeySequence("F3"));
    m_copyButton = new QPushButton(tr("F5 Копирование"), this);
    m_copyButton->setShortcut(QKeySequence("F5"));
    m_deleteButton = new QPushButton(tr("F8 Удаление"), this);
    m_deleteButton->setShortcut(QKeySequence("F8"));
    m_editButton = new QPushButton(tr("F9 Вырезание"), this);
    m_editButton->setShortcut(QKeySequence("F9"));

    int nButton = 4;
    int nBtnWidth = int(m_fmWidth/nButton);
    int nBtnHeight =  int(m_btnHeight);
    m_serverCopyFileSize = 0;

    m_viewButton->setMaximumHeight(nBtnHeight);
    m_viewButton->setMinimumWidth(nBtnWidth);
    m_copyButton->setMaximumHeight(nBtnHeight);
    m_copyButton->setMinimumWidth(nBtnWidth);
    m_deleteButton->setMaximumHeight(nBtnHeight);
    m_deleteButton->setMinimumWidth(nBtnWidth);
    m_editButton->setMaximumHeight(nBtnHeight);
    m_editButton->setMinimumWidth(nBtnWidth);

    QHBoxLayout* buttonsHL = new QHBoxLayout;
    QHBoxLayout* dialogHL = new QHBoxLayout;
    QVBoxLayout* mainVL = new QVBoxLayout;

    buttonsHL->addWidget(m_viewButton);
    buttonsHL->addWidget(m_copyButton);
    buttonsHL->addWidget(m_deleteButton);
    buttonsHL->addWidget(m_editButton);
    buttonsHL->setSpacing(5);

    dialogHL->addWidget(m_dialog1);
    dialogHL->addWidget(m_dialog2);
    dialogHL->setSpacing(5);

    mainVL->addLayout(dialogHL);
    mainVL->addLayout(buttonsHL);
    mainVL->setMargin(2);
    mainVL->setSpacing(5);

    setLayout(mainVL);

    connect(m_viewButton, &QPushButton::clicked, this, [this](){
        onViewButton();
    });
    connect(m_copyButton, &QPushButton::clicked, this, [this](){
        onCopyButton();
    });
    connect(m_deleteButton, &QPushButton::clicked, this, [this](){
        onDeleteButton();
    });
    connect(m_editButton, &QPushButton::clicked, this, [this](){
        onEditButton();
    });
    connect(m_dialog1, &HalfDlg::serverCall, this, [this](){
        if(!m_dialog1->isServer()) emitServerRequest(FMDataHeader::GetCatalog, 1);
    });
    connect(m_dialog2, &HalfDlg::serverCall, this, [this](){
        if(!m_dialog2->isServer()) emitServerRequest(FMDataHeader::GetCatalog, 2);
    });
    connect(m_dialog1, &HalfDlg::clientCall, this, [this](){
        if(m_dialog1->isServer()){
            m_dialog1->switchLabel(!m_dialog1->isServer());
            m_dialog1->setClient();
        }
    });
    connect(m_dialog2, &HalfDlg::clientCall, this, [this](){
        if(m_dialog2->isServer()){
            m_dialog2->switchLabel(!m_dialog2->isServer());
            m_dialog2->setClient();
        }
    });
    connect(m_dialog1, &HalfDlg::serverTreeItemClicked, this, [this](QString path){
        emitServerRequest(FMDataHeader::GetFileList, 1, path);
    });
    connect(m_dialog2, &HalfDlg::serverTreeItemClicked, this, [this](QString path){
        emitServerRequest(FMDataHeader::GetFileList, 2, path);
    });
    connect(m_dialog1, &HalfDlg::deleteFile, this, [this](QString path){
        emitServerRequest(FMDataHeader::DeleteFile, 1, path);
    });
    connect(m_dialog2, &HalfDlg::deleteFile, this, [this](QString path){
        emitServerRequest(FMDataHeader::DeleteFile, 2, path);
    });

    showFM();
}
//-------------------------------------------------------------------
FileManager::~FileManager(void)
{
    if(m_copyDlg)  m_copyDlg->deleteLater();
}
//-----------------------------------------------------------------------
void FileManager::showFM()
{
    show();
    m_dialog1->showDir();
    m_dialog2->showDir();
}
//-----------------------------------------------------------------------------------------------------
void FileManager::refreshManager()
{
    if(!m_dialog1->isServer())
        m_dialog1->showDir();
    else
        emit m_dialog1->serverTreeItemClicked(m_dialog1->getCurrentName());
    if(!m_dialog2->isServer())
        m_dialog2->showDir();
    else
        emit m_dialog2->serverTreeItemClicked(m_dialog2->getCurrentName());
    if(!m_copyButton->isEnabled())
        m_copyButton->setEnabled(true);
}
//------------------------------------------------------------------------
//void FileManager::showRecreateDlg(QString sName)
//{
//    if(!m_recreateDlg) {
//        m_recreateDlg = new RecreateDlg(this, sName);
//        connect(m_recreateDlg, &RecreateDlg::finished, this, [this](int) {
//            RefreshManager(0);
//        });
//    }
//    else  m_recreateDlg->showDlg(sName);
//}
//-------------------------------------------------------------------------------------------------
void FileManager::onViewButton()  // просмотр логов
{
    if(m_dialog1->isActive())  m_dialog1->preOnF3();
    else  m_dialog2->preOnF3();
}
//-----------------------------------------------------------------------------------------------------
void FileManager::onCopyButton()  // копирование файла
{
    if(m_dialog1->isActive())  m_dialog1->onF5(this);
    else  m_dialog2->onF5(this);
}
//-----------------------------------------------------------------------------------------------------
void FileManager::onDeleteButton()  // удалить файл
{
    int nIsRefresh; // удаление прошло удачно, нужно обновить списки файлов
    if(checkAccess()){
    if(m_dialog1->isActive()) nIsRefresh = m_dialog1->onF8();
    else nIsRefresh = m_dialog2->onF8();
    if(nIsRefresh)  refreshManager();
    }
}
//-----------------------------------------------------------------------------------------------------
void FileManager::onEditButton()  // вырезание файла
{
    if(m_dialog1->isActive())  m_dialog1->onF9(this);
    else  m_dialog2->onF9(this);
}
//------------------------------------------------------------------------------------------------------
void FileManager::recieveFMData(QByteArray arr)
{
    const FMDataHeader *header = reinterpret_cast<const FMDataHeader*>(arr.constData());
    arr = arr.remove(0, sizeof(FMDataHeader));
    QJsonDocument doc = QJsonDocument::fromJson(arr);
    if(doc.object().value("DialogNum").toInt() == 1) {
        m_dialog1->setServer();
        m_dialog1->switchLabel(m_dialog1->isServer());
    }
    else if(doc.object().value("DialogNum").toInt() == 2){
        m_dialog2->setServer();
        m_dialog2->switchLabel(m_dialog2->isServer());
    }
    QStringList strlist = doc.object().keys();

    switch (header->m_type) {
    case FMDataHeader::GetCatalog:
        for (int i = 0;i<strlist.size();i++) {
            if(strlist[i]!="DialogNum")
                m_serverCatalogList.insert(i,doc.object().value(strlist[i]).toString());
        }
        displayCatalogs(doc.object());
        break;
    case FMDataHeader::GetFileList:
        displayFileList(doc.object());
        break;
    case FMDataHeader::CopyFile:
    {
        QJsonDocument doc = QJsonDocument::fromJson(arr);
        if((doc.isObject())&&(doc.object().contains("FileSize"))){
            m_copyDlg->startRecievingThread(doc.object().value("FileSize").toVariant().toLongLong());
        }
        else if(doc.isObject()&doc.object().contains("CopyProcent")){
            m_copyDlg->setProcent(doc.object().value("CopyProcent").toInt());
        }
        else if(doc.isObject()&doc.object().contains("CopyStatus")){
            m_copyDlg->handleTheCopyStatus(doc.object().value("CopyStatus").toInt());
            emit refreshManager();
        }
        else if(doc.isObject()&doc.object().contains("SendNext")){
                m_copyDlg->goOnCopying();
        }
        else m_copyDlg->copy(arr);
        break;
    }
    case FMDataHeader::FinishCopy:
        if(doc.object().value("CopyStatus")==LocalCopyThread::CopyisSuccessful)
        m_copyDlg->finishTheCopy(doc.object().value("CopyStatus").toInt());
        break;
    case FMDataHeader::CancelCopy:
        m_copyDlg->cancelTheCopy();
        break;
    }
}
//-----------------------------------------------------------------------------------------
void FileManager::displayFileList(QJsonObject obj)
{
    QString path = obj.value("Path").toString();
    if(obj.value("DialogNum").toInt() == 2) {
        m_dialog2->setCurrentPath(path);
        m_dialog2->showServerDirs(obj, m_serverCatalogList);
    }
    else if(obj.value("DialogNum").toInt() == 1){
        m_dialog1->setCurrentPath(path);
        m_dialog1->showServerDirs(obj, m_serverCatalogList);
    }
}
//----------------------------------------------------------------------------------------------
void FileManager::displayCatalogs(QJsonObject obj)
{
    QStringList strlist = obj.keys();
    strlist.removeOne("DialogNum");
    if(obj.value("DialogNum").toInt() == 1){
        m_dialog1->setComboPaths(strlist);
        m_dialog1->setCurrentPath(m_serverCatalogList[0]);
        emitServerRequest(FMDataHeader::GetFileList, obj.value("DialogNum").toInt(), m_serverCatalogList[0]);
    }
    else if(obj.value("DialogNum").toInt() == 2){
        m_dialog2->setComboPaths(strlist);
        m_dialog2->setCurrentPath(m_serverCatalogList[0]);
        emitServerRequest(FMDataHeader::GetFileList, obj.value("DialogNum").toInt(), m_serverCatalogList[0]);
    }
}
//-------------------------------------------------------------------------------
void FileManager::emitServerRequest(int operation, int numHlf, QString pathFrom, QString pathWhere, int type, qint64 filesize)
{
    switch (operation) {
    case FMDataHeader::GetCatalog:
    {
        QJsonObject obj;
        obj["DialogNum"] = numHlf;
        sendFMData(operation, QJsonDocument(obj).toJson());
        break;
    }
    case FMDataHeader::GetFileList:
    {
        QJsonObject obj;
        obj["DialogNum"] = numHlf;
        obj["Path"] = pathFrom;
        sendFMData(operation, QJsonDocument(obj).toJson());
        break;
    }
    case FMDataHeader::CopyFile:
    {
        QJsonObject obj;
        obj["CopyFrom"] = pathFrom;
        obj["CopyTo"] = pathWhere;
        obj["CopyType"] = type;
        sendFMData(operation, QJsonDocument(obj).toJson());
        break;
    }
    case FMDataHeader::CancelCopy:
    {
        QJsonObject obj;
        obj["CopyType"] = numHlf;   //это не номер половинки а тип копирования
        QByteArray arr = QJsonDocument(obj).toJson();
        sendFMData(operation, arr);
        break;
    }
    case FMDataHeader::CopyFileSize:
    {
        QJsonObject obj;
        obj["CopyTo"] = pathWhere;
        obj["FileSize"] = filesize;
        sendFMData(operation, QJsonDocument(obj).toJson());
        break;
    }
    case FMDataHeader::DeleteFile:
    {
        QJsonObject obj;
        obj["FileToBeDeleted"] = pathFrom;
        QByteArray arr = QJsonDocument(obj).toJson();
        sendFMData(operation, arr);
        break;
    }
    }
}
//-----------------------------------------------------------------------------
void FileManager::emitServerRequest(int operation, QString pathFrom, QString pathWhere, int type)
{
    QJsonObject obj;
    obj["CopyFrom"] = pathFrom;
    obj["CopyTo"] = pathWhere;
    obj["CopyType"] = type;
    sendFMData(operation, QJsonDocument(obj).toJson());
}

void FileManager::emitServerRequest(int operation, QString pathWhere, qint64 fileSize)
{
    QJsonObject obj;
    obj["CopyTo"] = pathWhere;
    obj["FileSize"] = fileSize;
    sendFMData(operation, QJsonDocument(obj).toJson());
}
//---------------------------------------------------------------------------------
void FileManager::emitServerRequest(int operation, int copyType, int status)
{
    QJsonObject obj;
    obj["CopyType"] = copyType;
    obj["FinishStatus"] = status;
    QByteArray arr = QJsonDocument(obj).toJson();
    sendFMData(operation, arr);
}
//-------------------------------------------------------------------------------------------------------
void FileManager::sendFMData(uint type, QByteArray data)
{
    FMDataHeader header(type);
    data.prepend(reinterpret_cast<const char*>(&header), sizeof(header));
    emit FMDataSended(data);
}
//--------------------------------------------------------------------------------------
bool FileManager::checkAccess()
{
    return 1;
}
//----------------------------------------------------------------------------------------------------
int FileManager::showCopyDlg(QString sCpyFrom, QString sCpyTo, uint type)
{
    if(!m_copyDlg) {
        m_copyDlg = new CopyDialog(this, type);
        m_copyDlg->setFilePaths(sCpyFrom, sCpyTo);
        if(m_dialog1->isActive()){
            m_copyDlg->setClientServerStatusFrom(m_dialog1->isServer());
            m_copyDlg->setClientServerStatusTo(m_dialog2->isServer());
        }
        else{
            m_copyDlg->setClientServerStatusFrom(m_dialog2->isServer());
            m_copyDlg->setClientServerStatusTo(m_dialog1->isServer());
        }

        m_copyDlg->showDlg();

        connect(m_copyDlg, &CopyDialog::copyIsFinished, this, [this](){
            m_copyButton->setEnabled(true);
            if(m_dialog1->isActive())  m_dialog1->copyIsOver();
            else  m_dialog2->copyIsOver();
        });
        connect(m_copyDlg, &CopyDialog::finished, this, [this]{
            m_copyButton->setEnabled(true);
            if(m_dialog1->isActive())  m_dialog1->copyIsOver();
            else  m_dialog2->copyIsOver();
        });
        connect(m_copyDlg, &CopyDialog::rejected, this, [this]{
            m_copyButton->setEnabled(true);
            if(m_dialog1->isActive())  m_dialog1->copyIsOver();
            else  m_dialog2->copyIsOver();
        });
        connect(m_copyDlg, &CopyDialog::refreshCopyDlg, this, &FileManager::refreshManager);

        connect(m_copyDlg, &CopyDialog::serverCopy, this, [this](QString copyFrom, QString copyTo, uint type){
            emitServerRequest(FMDataHeader::CopyFile, copyFrom, copyTo, type);
        });
        connect(m_copyDlg, &CopyDialog::sendCopyPack, this, [this](QByteArray arr){
            sendFMData(FMDataHeader::CopyFile, arr);
        });
        connect(m_copyDlg, &CopyDialog::readyToRecieveNext, this, [this](QByteArray arr){
            sendFMData(FMDataHeader::CopyFile, arr);
        });
        connect(m_copyDlg, &CopyDialog::sendFileSize, this, [this](qint64 fileSize, QString copyTo){
            emitServerRequest(FMDataHeader::CopyFileSize, copyTo, fileSize);
        });
        connect(m_copyDlg, &CopyDialog::cancelServerToClientCopy, this, [this](){
            emitServerRequest(FMDataHeader::CancelCopy, LocalCopyThread::ServerToClient);
        });
        connect(m_copyDlg, &CopyDialog::cancelServerToServerCopy, this, [this](){
            emitServerRequest(FMDataHeader::CancelCopy, LocalCopyThread::ServerToServer);
        });
        connect(m_copyDlg, &CopyDialog::cancelClientToServerCopy, this, [this](){
            emitServerRequest(FMDataHeader::CancelCopy, LocalCopyThread::ClientToServer);
        });
        connect(m_copyDlg, &CopyDialog::finishClientToServerCopy, this, [this](int status){
            emitServerRequest(FMDataHeader::FinishCopy, LocalCopyThread::ClientToServer, status);
        });
        connect(m_copyDlg, &CopyDialog::finishServerToClientCopy, this, [this](int status){
            emitServerRequest(FMDataHeader::FinishCopy, LocalCopyThread::ServerToClient, status);
        });
    }
    else {
        m_copyDlg->setCopyingType(type);
        m_copyDlg->setFilePaths(sCpyFrom, sCpyTo);
        if(m_dialog1->isActive()){
            m_copyDlg->setClientServerStatusFrom(m_dialog1->isServer());
            m_copyDlg->setClientServerStatusTo(m_dialog2->isServer());
        }
        else{
            m_copyDlg->setClientServerStatusFrom(m_dialog2->isServer());
            m_copyDlg->setClientServerStatusTo(m_dialog1->isServer());
        }
        m_copyDlg->showDlg();
    }
    m_copyButton->setDisabled(true);

    return 0;
}
