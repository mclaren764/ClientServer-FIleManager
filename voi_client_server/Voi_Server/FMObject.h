#ifndef FMOBJECT_H
#define FMOBJECT_H

#include <QObject>
#include <QtWidgets/QWidget>
#include <RmoTypes.h>
#include <ModuleServerRmo.h>
#include <Common/LocalCopyThread.h>
#include <Common/RecievingCopyThread.h>
#include <QJsonObject>
#include <Common/SendingCopyThread.h>


class FMObject : public QObject
{
    Q_OBJECT

    QJsonObject m_catalogList;
    LocalCopyThread* m_localServerCopyThread;
    SendingCopyThread* m_sendingCopier;
    RecievingCopyThread* m_recievingCopier;
    QThread m_recievingCopyThread;
    QThread m_sendingCopyThread;

    int m_copyStatus;

public slots:
    void cancelSending(){if(m_sendingCopyThread.isRunning()) m_sendingCopier->cancelCopying();}
    void cancelLocalCopy(){if(m_localServerCopyThread) m_localServerCopyThread->isCanceled();}
    void cancelRecieving(){if(m_recievingCopier) m_recievingCopier->cancelCopying();};
    void copyStatus(int status){m_copyStatus = status;}
    void copy(QByteArray arr){if(m_recievingCopier) m_recievingCopier->doCopy(arr);}
    void sendStatusCopy(){
        QJsonObject obj
        {
            {"CopyStatus", m_copyStatus},
            {"CopyTo", ""}
        };
        QJsonDocument doc = QJsonDocument(obj);
        emit sendCopyStatus(doc.toJson());
    }

public:
    FMObject();
    //~FMObject();
    void receiveFMCommandFromRmo(QByteArray, uint); // команды от РМО для Файлового менеджера
    void sendCatalogs(QJsonObject);
    void sendFileList(QByteArray);
    void startCopyThread(QByteArray);
    void startSendingThread(QJsonObject);
    void startRecievingThread(QString, qint64);
    void startLocalThread(QJsonObject);
    void deleteFile(QJsonObject);
    void cancelCopying(QJsonDocument);

signals:
    void FMFileDataSended(QByteArray);
    void FMCatalogDataSended(QByteArray);
    void sendFileSize(QByteArray);
    void sendCopyPack(QByteArray);
    void stopThread();
    void cancelCopy();
    void sendProcent(QByteArray);
    void sendCopyStatus(QByteArray);
    void readyToRecieveNext(QByteArray);
    void finishCopy(QByteArray);
};

#endif // FMOBJECT_H
