#ifndef SENDINGCOPYTHREAD_H
#define SENDINGCOPYTHREAD_H

#include <QThread>
#include <QFile>
#include <QDebug>
#include <Common/LocalCopyThread.h>

class SendingCopyThread : public QObject
  {
  Q_OBJECT
    QString m_copyFrom; //Путь откуда копируется файл
    QString m_copyTo;   //Путь куда копируется файл
    qint64 m_fileSize;
    bool m_copyCancel;  //Статус отмены копирования
    QFile m_fileFrom;
    int m_status;

public:
    SendingCopyThread(QString copyFrom){
        m_copyCancel = false;
        m_copyFrom = copyFrom;
        m_fileFrom.setFileName(m_copyFrom);
        m_fileSize = m_fileFrom.size();
        if (!m_fileFrom.open(QIODevice::ReadOnly)){
            m_status = LocalCopyThread::ReadFileCannotBeOpened;
            return;
        }
    };
    ~SendingCopyThread(){
    }

public slots:
  void doSend();
  void isCanceled(){m_copyCancel = true;};
  void stopCopying(int status);
  void cancelCopying();
  void getFileSize(){
      emit sendFileSize(m_fileSize);}
  int getStatus(){return m_status;}

signals:
  void copyIsFinishing(int status);
  void procentReg(int procent) const;
  void copyStatus(uint flag);
  void sendCopyPack(QByteArray);
  void sendFileSize(qint64);
  void finishCopy();
};

#endif // TESTTCPCOPYTHREAD_H
