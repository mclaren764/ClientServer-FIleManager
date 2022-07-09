#include "FMObject.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <Enums.h>
#include <QJsonArray>
#include <QtWidgets/QTreeWidgetItem>
#include <QDateTime>
#include <Common/SendingCopyThread.h>
#include <Common/LocalCopyThread.h>
#include <Common/Utils/AsyncFileUtils.h>
#include <RmoTypes.h>

using namespace AsyncFileUtils;

FMObject::FMObject()
{
    m_localServerCopyThread = nullptr;
    m_sendingCopier = nullptr;
    m_recievingCopier = nullptr;
    m_catalogList.insert("NPO", "/home/local/NPO");
    m_catalogList.insert("media", "/media");
    m_catalogList.insert("my", "/my");
}
//---------------------------------------------------------------------------------
void FMObject::receiveFMCommandFromRmo(QByteArray arr, uint type)
{
    QJsonDocument doc = QJsonDocument::fromJson(arr);
    switch (type) {
    case FMDataHeader::GetCatalog:
        sendCatalogs(QJsonDocument::fromJson(arr).object());
        break;
    case FMDataHeader::GetFileList:
        sendFileList(arr);
        break;
    case FMDataHeader::CopyFile:
        startCopyThread(arr);
        break;
    case FMDataHeader::CopyFileSize:
        startRecievingThread(doc.object().value("CopyTo").toString(), doc.object().value("FileSize").toVariant().toLongLong());
        break;
    case FMDataHeader::CancelCopy:
        cancelCopying(doc);
        break;
    case FMDataHeader::FinishCopy:
        if(m_recievingCopier)
            m_recievingCopier->stopCopying(doc.object().value("FinishStatus").toInt());
        else if(m_sendingCopier)
            m_sendingCopier->stopCopying(doc.object().value("FinishStatus").toInt());
        break;
    case FMDataHeader::DeleteFile:
        deleteFile(doc.object());
        break;
    }
}
//---------------------------------------------------------------------------------
void FMObject::sendCatalogs(QJsonObject catalogDirs)
{
    m_catalogList.insert("DialogNum", catalogDirs.value("DialogNum"));
    QByteArray arr = QJsonDocument(m_catalogList).toJson();
    emit FMCatalogDataSended(arr);
}
//---------------------------------------------------------------------------------------
QString fileSize(long nSize) // Вспомогательная ф-ия перевода размера файла в строку
{
    QString str;
    int i = 0;
    for(i = 0; nSize > 1023; nSize /= 1024, i++) {}
    return str.setNum(nSize) + "BKMGT"[i];
}
//--------------------------------------------------------------------------------------------
void FMObject::sendFileList(QByteArray clickedPath)
{
    QJsonDocument docRec = QJsonDocument::fromJson(clickedPath);
    QJsonObject obj = docRec.object();
    QString path = obj.value("Path").toString();
    //Если нажали на "UP"
    if(path.contains("...")){
        path.remove(path.size()-4, 4);
        path = path.left(path.toUtf8().lastIndexOf('/'));
        obj.insert("Path" , path);
    }
    QDir dir = QDir(path);
    QStringList  list = dir.entryList(QDir::Dirs);
    QStringList::Iterator it;
    QJsonArray jsarrDirs;
    QFileInfo qFI;
    for( it=list.begin();  it != list.end(); ++it )  {
        qFI.setFile(dir, *it);
        QJsonObject object;
        object[*it]=qFI.lastModified().toString();
        jsarrDirs.insert(jsarrDirs.size(),object);
     }
    obj.insert("Dirs" , jsarrDirs);
    list = dir.entryList(QDir::Files);
    QJsonArray jsarrFiles;
    for( it=list.begin();  it != list.end(); ++it )  {
        qFI.setFile(dir, *it);
        QJsonObject object;
        object["Name"] = qFI.completeBaseName();
        object["Extension"] = qFI.suffix();
        object["Size"] = fileSize(qFI.size());
        object["Data"] = qFI.lastModified().toString();
        jsarrFiles.insert(jsarrFiles.size(),object);
     }
    obj.insert("Files" , jsarrFiles);
    QJsonDocument docSend = QJsonDocument(obj);

    emit FMFileDataSended(docSend.toJson());
}
//----------------------------------------------------------------------------------------
void FMObject::startSendingThread(QJsonObject obj)
{
    m_sendingCopier = new SendingCopyThread(obj.value("CopyFrom").toString());
    m_sendingCopier->moveToThread(&m_sendingCopyThread);
    connect(m_sendingCopier, &SendingCopyThread::sendFileSize, this, [this](qint64 fileSize){
        QJsonObject obj
        {
            {"FileSize", fileSize},
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit sendFileSize(doc.toJson());
    });
    connect(m_sendingCopier, &SendingCopyThread::sendCopyPack, this, [this](QByteArray arr){
        emit sendCopyPack(arr);
    });
    connect(m_sendingCopier, &SendingCopyThread::copyStatus, this, &FMObject::copyStatus);
    connect(m_sendingCopier, &SendingCopyThread::copyIsFinishing, this,  [this](int status) {
        QJsonObject obj
        {
            {"CopyStatus", status},
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit sendCopyStatus(doc.toJson());

        if(m_sendingCopier) {
            m_sendingCopier->deleteLater();
            m_sendingCopier = nullptr;
        }
        m_sendingCopyThread.quit();
        m_sendingCopyThread.wait();
    });
    if(m_sendingCopier->getStatus() == LocalCopyThread::ReadFileCannotBeOpened){
        m_sendingCopier->stopCopying(m_sendingCopier->getStatus());
        return;
    }
    m_sendingCopyThread.start();
    m_sendingCopier->getFileSize();
}
//-------------------------------------------------------------------------------------------------------------
void FMObject::startRecievingThread(QString copyTo, qint64 size)
{
    m_recievingCopier = new RecievingCopyThread(copyTo, size);
    m_recievingCopier->moveToThread(&m_recievingCopyThread);
    connect(m_recievingCopier, &RecievingCopyThread::sendNextCopyPack, this, [this](){
        QJsonObject obj
        {
            {"SendNext", true},
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit readyToRecieveNext(doc.toJson());
    });
    connect(m_recievingCopier, &RecievingCopyThread::procentReg, this, [this](int procent){
        QJsonObject obj
        {
            {"CopyProcent", procent},
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit sendProcent(doc.toJson());
    });
    connect(m_recievingCopier, &RecievingCopyThread::copyStatus, this, &FMObject::copyStatus);
    connect(m_recievingCopier, &RecievingCopyThread::copyIsFinishing, this,  [this, copyTo](int status) {
        QJsonObject obj
        {
            {"CopyStatus", status},
            {"CopyTo", copyTo}
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit sendCopyStatus(doc.toJson());
        if(m_recievingCopier) {
            m_recievingCopier->deleteLater();
            m_recievingCopier = nullptr;
        }
        m_recievingCopyThread.quit();
        m_recievingCopyThread.wait();
    });
    if(m_recievingCopier->getStatus()!=0){
        m_recievingCopier->stopCopying(m_recievingCopier->getStatus());
        return;
    }
    m_recievingCopyThread.start();
    emit m_recievingCopier->sendNextCopyPack();

}
//----------------------------------------------------------------------------------------------------------------
void FMObject::startCopyThread(QByteArray arr)
{
    QJsonDocument docRec = QJsonDocument::fromJson(arr);
    // если не объект Json, то пришли данные для записи в файл
    if(!(docRec.isObject()))
        copy(arr);
    QJsonObject obj = docRec.object();
    if(obj.contains("SendNext")&& m_sendingCopier)
        m_sendingCopier->doSend();
    else{
    switch (obj.value("CopyType").toInt()) {
    case LocalCopyThread::ServerToClient:
        startSendingThread(obj);
        break;
    case LocalCopyThread::ServerToServer:
        startLocalThread(obj);
        break;
    }
    }
}
//--------------------------------------------------------------------------------
void FMObject::startLocalThread(QJsonObject object)
{
    QString str = object.value("CopyTo").toString();
    m_localServerCopyThread = new LocalCopyThread(object.value("CopyFrom").toString(), object.value("CopyTo").toString());
    connect(m_localServerCopyThread, &LocalCopyThread::procentReg, this, [this](int procent){
        QJsonObject obj
        {
            {"CopyProcent", procent},
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit sendProcent(doc.toJson());
    });
    connect(this, &FMObject::cancelLocalCopy, m_localServerCopyThread, &LocalCopyThread::isCanceled);
    connect(m_localServerCopyThread, &LocalCopyThread::copyStatus, this, &FMObject::copyStatus);
    connect(m_localServerCopyThread, &LocalCopyThread::finished, this,  [this, object]() {
        QJsonObject obj
        {
            {"CopyStatus", m_copyStatus},
            {"CopyTo", object.value("CopyTo").toString()}
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit sendCopyStatus(doc.toJson());
        m_localServerCopyThread->deleteLater();
        m_localServerCopyThread = nullptr;
    });
    m_localServerCopyThread->start();
}
//----------------------------------------------------------------------------------------
void FMObject::deleteFile(QJsonObject obj)
{
    deleteFileAsync(obj.value("FileToBeDeleted").toString());
}
//------------------------------------------------------------------------------------------
void FMObject::cancelCopying(QJsonDocument doc)
{
    if(doc.object().value("CopyType").toInt()==LocalCopyThread::ServerToClient)
        cancelSending();
    else if(doc.object().value("CopyType").toInt()==LocalCopyThread::ServerToServer)
        cancelLocalCopy();
    else if(doc.object().value("CopyType").toInt()==LocalCopyThread::ClientToServer)
        m_recievingCopier->cancelCopying();
}
//---------------------------------------------------------------------------------
