#ifndef HALFDLG_H
#define HALFDLG_H

#include <QWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QLabel>
#include <QWidget>
#include <QPixmap>
#include <FileManager.h>
#include <QJsonObject>
#include <LogWindow.h>

class HalfDlg : public QWidget
  {
  Q_OBJECT
    QTreeWidget *m_fileManagerList;          // Столбцы диалога
    QComboBox *m_comboPaths;           // Раскрывающийся список
    QComboBox *m_clientServer;    // Список для переключения между клиентом и сервером
    QLabel *m_currentPath;     // Окно для отображения текущего пути
    QLabel *m_clientServerDisp;   // Окно отображение переключения между клиентом и сервером
    static QPixmap *m_imageList;  // Список с иконками
    QString m_currentName;        // Строка с последней правильной (существующей) директорией
    HalfDlg *m_otherDlg;          // Указатель на другую половинку
    bool m_isActive;              // Флаг указывающий на окно с которым происходит взаимодействие
    QString m_isFileCopying;      // Путь на файл который в данный момент копируется
    QString m_isFileViewing;      // Путь на файл который в данный момент просматривается
    bool m_isServer;              // Флаг указывающий на то, переключена ли половинка на сервер
    QJsonObject m_serverComboBoxObj;
    LogWindow *m_logWnd;

  public slots:
    void changeSelectCombo(int nPos);
    void clientServer(int);
    void clickDirOrFile( QTreeWidgetItem* pItem, int nColumn);  // Кликнули по cписку два раза
    void showDir();
    void showServerDirs(QJsonObject, QStringList);
    void setActive(QTreeWidgetItem*, int) {m_isActive = true;  if(m_otherDlg)  m_otherDlg->clearSelection();}
    void switchLabel(bool sw);  //Изменение надписи в окне переключения
    void setCurrentPath(QString path){m_currentPath->setText(path);}
    void setComboPaths(QStringList);
    void setServer(){m_isServer = true;}
    void setClient(){m_isServer = false;}
    void copyIsOver(){m_isFileCopying = "";}
    void viewIsOver(){m_isFileViewing = "";}

  public:
    enum {eDIR_UP, eDIR, eDEF, eLOG, eKT, eTE, eDIR_LAN};
    HalfDlg(QWidget* parent);
    ~HalfDlg() { }
    void preOnF3();
    void onF3(const QString sFileName);
    void onF5(FileManager *pFM);
    int onF8();
    void onF9(FileManager *pFM);// Окно вырезания
    bool isActive() { return m_isActive; }
    void clearSelection() { m_isActive = false; m_fileManagerList->clearSelection(); }
    QString getCurrentName() { return m_currentName; }
    QString getFileFromList(const QString operation);
    HalfDlg *getOtherDlg() { return m_otherDlg; }
    void setOtherDlg(HalfDlg *pDlg) { m_otherDlg = pDlg; }
    int getIconFromExt(QString sExt);
    bool checkConfig(const QString fExt, const QString operation);
    bool isServer(){return m_isServer;}

signals:
    void serverCall();
    void clientCall();
    void serverTreeItemClicked(QString);
    void deleteFile(QString);

};

#endif // HALFDLG_H
