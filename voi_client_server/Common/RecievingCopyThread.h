#ifndef RECIEVINGCOPYTHREAD_H
#define RECIEVINGCOPYTHREAD_H

#include <QThread>
#include <QFile>
#include <QDebug>
#include "LocalCopyThread.h"

class RecievingCopyThread : public QObject
  {
  Q_OBJECT
    QString m_copyFrom; //Путь откуда копируется файл
    QString m_copyTo;   //Путь куда копируется файл
    qint64 m_fileSize;
    QFile m_fileTo;
    int m_status;

public:
    RecievingCopyThread(QString copyTo, qint64 size){
        m_copyTo = copyTo;
        m_fileSize = size;
        m_fileTo.setFileName(m_copyTo);
        if (!m_fileTo.open(QIODevice::WriteOnly)){
            m_status = LocalCopyThread::WriteFileCannotBeOpened;
            return;
        }
    };
    ~RecievingCopyThread(){}

public slots:
  void doCopy(QByteArray arr);
  void cancelCopying();
  void stopCopying(int status);
  int getStatus(){return m_status;}

signals:
  void copyIsFinishing(int copyStatus);
  void procentReg(int procent) const;
  void copyStatus(int flag);
  void sendCopyPack(QByteArray);
  void sendFileSize(int);
  void sendNextCopyPack();

};


#endif // RECIEVINGCOPYTHREAD_H
