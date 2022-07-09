#ifndef LOCALCOPYTHREAD_H
#define LOCALCOPYTHREAD_H
#include <QThread>
#include <QDebug>

class LocalCopyThread : public QThread
  {
      Q_OBJECT
private:
QString m_copyFrom; //Путь откуда копируется файл
QString m_copyTo;   //Путь куда копируется файл
bool m_copyCancel;  //Статус отмены копирования

public:
    LocalCopyThread(const QString& CopyFrom = "", const QString& CopyTo = ""){
        m_copyFrom = CopyFrom;
        m_copyTo = CopyTo;
        m_copyCancel = false;
    };
    ~LocalCopyThread(){};
    void run() override;
    enum CopyType{ClientToClient = 0, ClientToServer, ServerToClient, ServerToServer};
    enum CopyStatus{CopyisSuccessful = 0, CopyisCanceled, ReadFileCannotBeOpened,
               WriteFileCannotBeOpened, ReadFileError, WriteFileError};
  signals:
      void procentReg(int procent);
      void copyStatus(int flag);

  public slots:
      void isCanceled(){m_copyCancel = true;}
  };
#endif // COPYINGTHREAD_H
