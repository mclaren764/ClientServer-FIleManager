#ifndef CPYDLG_H
#define CPYDLG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QProgressDialog>
#include <QFile>
#include <FileManager.h>
#include <LocalCopyThread.h>
#include <RecievingCopyThread.h>
#include <SendingCopyThread.h>
#include <QProgressBar>

class CopyDialog : public QDialog
  {
  Q_OBJECT
    QPushButton *m_cancelButton;
    QPushButton *m_okButton;
    QLabel *m_textLabelFrom;          // Cтатический текст "Из:..."
    QLabel *m_textLabelTo;         // Cтатический текст "в:"
    QLabel *m_textLabelClientServerFrom;  //Статический текст отображение клиента или сервера
    QLabel *m_textLabelClientServerTo;  //Статический текст отображение клиента или сервера
    QLineEdit   *m_editLine;          // Окно ввода
    QProgressBar *m_copyProgressBar;  // Полоска прогресса копирования
    LocalCopyThread *m_localCopyThread;      // Отдельный поток для копирования
    QThread m_recievingCopyThread;
    QThread m_sendingCopyThread;
    RecievingCopyThread* m_recievingCopier;
    SendingCopyThread* m_sendingCopier;
    QString m_copyFrom, m_copyTo; // Пути копируемого файла и файла назначения
    bool m_cancelCopy;                  // Отмена копирования
    int m_copyStatus;                // Статус копирования
    int m_copyingType;               //Откуда и куда происходит копирование(Клиент/Сервер)
    QByteArray m_copyingData;        //Данные пришедшие с сервера

 signals:
    void cancelLocalCopy();
    void cancelServerToClientCopy();
    void cancelServerToServerCopy();
    void cancelClientToServerCopy();
    void refreshCopyDlg();  // Обновление файлового менеджера
    void serverCopy(QString = "", QString = "", uint = 0);
    void finishTheCopy(int status);
    void copyIsFinished();
    void sendCopyPack(QByteArray);
    void sendFileSize(qint64, QString);
    void finishClientToServerCopy(int copyStatus);
    void finishServerToClientCopy(int copyStatus);
    void cancelTheCopy();
    void readyToRecieveNext(QByteArray);

 public slots:
    virtual void accept();
    virtual void reject();
    void setProcent(int procent){m_copyProgressBar->setValue(procent);}
    void copyStatus(int status){m_copyStatus = status;}
    void copy(QByteArray arr){if(m_recievingCopier) m_recievingCopier->doCopy(arr);}
    void onCopyThreadFinished(const QString filePath, int copyStatus);
    void goOnCopying(){if(m_sendingCopier)m_sendingCopier->doSend();}
    void handleTheCopyStatus(int);

  public:
    CopyDialog(FileManager *parent, uint);
    ~CopyDialog();
    void showDlg();        // Отображение диалога
    void startLocalThread();
    void startRecievingThread(qint64 size = 0);
    void startSendingThread();
    void setFilePaths(const QString copyFromPath, const QString copyToPath){m_copyFrom = copyFromPath; m_copyTo = copyToPath;}
    void setClientServerStatusFrom(bool isServer){m_textLabelClientServerFrom->setText(isServer?"Сервер":"Клиент");}
    void setClientServerStatusTo(bool isServer){m_textLabelClientServerTo->setText(isServer?"Сервер":"Клиент");}
    void setCopyingType(uint type){m_copyingType = type;}
    void copyData(QByteArray arr){m_copyingData.swap(arr);}
   };

#endif // CPYDLG_H
