#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QWidget>
#include <QPushButton>
#include <QSplitter>
#include <QMessageBox>
#include <QJsonObject>
#include <QDialog>

class HalfDlg;
class CopyDialog;
class RecreateDlg;
class FileManager : public QWidget
{
    Q_OBJECT
    QPushButton* m_viewButton;
    QPushButton* m_editButton;
    QPushButton* m_copyButton;
    QPushButton* m_deleteButton;
    QSplitter* m_splitter;
    HalfDlg* m_dialog1;
    HalfDlg* m_dialog2;
    CopyDialog* m_copyDlg;
    RecreateDlg* m_recreateDlg;
    QStringList m_serverCatalogList;
    int m_serverCopyFileSize;

  public slots:
    void onViewButton();
    void onCopyButton();
    void onDeleteButton();
    void onEditButton();
    void refreshCalledFromCopyDlg(){refreshManager();};
    void sendFMData(uint type, QByteArray data);
    void recieveFMData(QByteArray);
    void displayFileList(QJsonObject);
    void displayCatalogs(QJsonObject);
    void emitServerRequest(int operation = 0, int numHlf = 0, QString pathFrom = "",
                           QString pathWhere = "", int copyType = 0, qint64 size = 0);

    void emitServerRequest(int operation, QString pathFrom,
                           QString pathWhere, int copyType);   // запуск копирования с сервера на клиент или с сервера на сервер

    void emitServerRequest(int operation, QString pathWhere, qint64);   //отправка размера файла перед копированием
    void emitServerRequest(int operation, int copyType, int status);   //Завершение копирования с определенным статусом

public:
    FileManager(QWidget* parent);
    ~FileManager();
    void showFM();
    void refreshManager();
    //void refreshServerManager(QStringList);
    //void showRecreateDlg(QString sName);  // Вызов окна вырезания (Необходимо реализовать вырезание)
    bool checkAccess();
    int showCopyDlg(QString sCpyFrom, QString sCpyTo, uint type);
    QStringList getServerCatalogList(){return m_serverCatalogList;}
    int getFileSize(){return m_serverCopyFileSize;}

signals:
    void FMDataSended(QByteArray);
    void sendCopyRequest(QString);
    void startCopyingThread();
    void stopCopyingThread();
  };

#endif // FILEMANAGER_H


