#include "HalfDlg.h"
#include <FileManager.h>
#include <Common/Utils/AsyncFileUtils.h>
#include <LogWindow.h>
#include <VoiTypes.h>
#include <RmoTypes.h>
#include "CopyDialog.h"

#include <QDir>
#include <QApplication>
#include <QTimer>
#include <QMessageBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const char *strDefFile_xpm[]={
"16 16 4 1",
"b c #000000",
"# c #808080",
". c None",
"a c #ffffff",
"................",
"..#######.......",
"..#aaaaa##......",
"..#aaaaa#a#.....",
"..#aaaaa#aa#....",
"..#aaaaa#####...",
"..#aaaaaaaaa#b..",
"..#aaaaaaaaa#b..",
"..#aaaaaaaaa#b..",
"..#aaaaaaaaa#b..",
"..#aaaaaaaaa#b..",
"..#aaaaaaaaa#b..",
"..#aaaaaaaaa#b..",
"..#aaaaaaaaa#b..",
"..###########b..",
"...bbbbbbbbbbb.."};

//-------------------------------------------------------------
/* XPM */
const char *strDir_xpm[]={
"16 16 5 1",
"c c #000000",
"# c #808080",
". c None",
"a c #ffff00",
"b c #ffffff",
"................",
"..#######.......",
".##aaaaa##......",
".#a.....a#####..",
".#bbbbbbbaaaa#..",
".#bbbbbbaaaaa#c.",
".#aaaaaaaaaaa#c.",
".#aaaaaaaaaaa#c.",
".#aaaaaaaaaaa#c.",
".#aaaaaaaaaaa#c.",
".#aaaaaaaaaaa#c.",
".#aaaaaaaaaaa#c.",
".#############c.",
"...cccccccccccc.",
"................",
"................"};

//-------------------------------------------------------------
/* XPM */
const char *strKtFile_xpm[]={
"16 16 5 1",
"c c #000000",
"a c #0000ff",
"# c #808080",
". c None",
"b c #ffff00",
"................",
"...##########...",
"..#aaaaaaaaaa#..",
"..#aaabaabaaa#c.",
"..#aabbababaa#c.",
"..#aaabababaa#c.",
"..#aaabaabaaa#c.",
"..#aaaaaaaaaa#c.",
"..#aaaaaaaaaa#c.",
"..#aaabaaabaa#c.",
"..#aabababbaa#c.",
"..#aababaabaa#c.",
"..#aaabaaabaa#c.",
"..#aaaaaaaaaa#c.",
"...##########cc.",
"....cccccccccc.."};

//-------------------------------------------------------------
/* XPM */
const char *strLogFile_xpm[]={
"16 16 5 1",
"b c #000000",
"a c #00ffff",
"# c #808080",
". c None",
"c c #ffff00",
"................",
"..#######.......",
"..#aaaaa##......",
"..#aaaaa#a#.....",
"..#a###a#aa#....",
"..#aaaaa#####...",
"..#a####aaaa#b..",
"..#aaaaaaaaa#b..",
"..#c##c####c#b..",
"..#ccccccccc#b..",
"..#c####c##c#b..",
"..#ccccccccc#b..",
"..#c##c####c#b..",
"..#ccccccccc#b..",
"..###########b..",
"...bbbbbbbbbbb.."};

//-------------------------------------------------------------
/* XPM */
const char *strNetDisk_xpm[]={
"16 16 5 1",
"c c #000080",
"# c #808080",
". c None",
"a c #ffff00",
"b c #ffffff",
"................",
"..#######.......",
".##aaaaa##......",
".#a.....a#####..",
".#bbbbbbbaaaa#..",
".#bbbbbbaaaaa#..",
".#aaaaaaaaaaa#..",
".#aaaaaaaaaaa#..",
".#aaaaaaaaaaa#..",
".#aaaaaaaaaaa#..",
".#aaaaaaaaaaa#..",
".#ccccccccccc#..",
".######c######..",
".......b........",
"..ccccbcbcccc...",
".......b........"};

//-----------------------------------------------------------------
const char *strReturn_xpm[]={
"16 16 3 1",
"# c #000000",
"a c #000080",
". c None",
"................",
"................",
"................",
"....#...........",
"...#a#..........",
"...#a#..........",
"..#aaa#.........",
".#a#a#a#........",
"..##a##.........",
"...#a#..........",
"...#a#..........",
"...#a###.##.##..",
"...#aaa#.a#.a#..",
"....####.##.##..",
"................",
"................"};

//-------------------------------------------------------------
/* XPM */
const char *strTeFile_xpm[]={
"16 16 5 1",
"c c #000000",
"b c #0000ff",
"# c #808080",
". c None",
"a c #ffffff",
"................",
"...##########...",
"..#aaaaaaaaaa#..",
"..#aaabaabaaa#c.",
"..#aabbababaa#c.",
"..#aaabababaa#c.",
"..#aaabaabaaa#c.",
"..#aaaaaaaaaa#c.",
"..#aaaaaaaaaa#c.",
"..#aaabaaabaa#c.",
"..#aabababbaa#c.",
"..#aababaabaa#c.",
"..#aaabaaabaa#c.",
"..#aaaaaaaaaa#c.",
"...##########cc.",
"....cccccccccc.."};

using namespace AsyncFileUtils;

QPixmap* HalfDlg::m_imageList = 0;

//-----------------------------------------------------
void removeEndSlash(QString& str)
{
    if((str[str.length() - 1] == '/') || (str[str.length() - 1] == '\\') ) str = str.left(str.length() - 1);
}
//-----------------------------------------------------

static const QPixmap  *pqpDir = 0;
static const QPixmap  *pqpDefFile = 0;
static const QPixmap  *pqpReturn = 0;
static const QPixmap  *pqpLogFile = 0;
static const QPixmap  *pqpKtFile = 0;
static const QPixmap  *pqpTeFile = 0;
static const QPixmap  *pqpNetDisk = 0;

HalfDlg::HalfDlg(QWidget* parent):QWidget(parent)  // конструктор окна - половинки ф.м., которые разделены сплиттером
{
    setMinimumSize(400, 400);
    int nCurrent = 4; //my

    pqpDir = new QPixmap (strDir_xpm);
    pqpDefFile = new QPixmap (strDefFile_xpm);
    pqpReturn = new QPixmap (strReturn_xpm);
    pqpLogFile = new QPixmap (strLogFile_xpm);
    pqpKtFile = new QPixmap (strKtFile_xpm);
    pqpTeFile = new QPixmap (strTeFile_xpm);
    pqpNetDisk = new QPixmap (strNetDisk_xpm);
    if(!m_imageList) { // этот код будет отрабатывать только один раз при первом вызове функции
        m_imageList = new QPixmap[7];
        m_imageList[0] = *pqpReturn;  //pkb_fm1
        m_imageList[1] = *pqpDir;
        m_imageList[2] = *pqpDefFile;
        m_imageList[3] = *pqpLogFile;
        m_imageList[4] = *pqpKtFile;
        m_imageList[5] = *pqpTeFile;
        m_imageList[6] = *pqpNetDisk;
    }
    m_currentPath = new QLabel(this);
    m_currentPath->setText("/my"/*QDir::homePath()*/);
    m_clientServerDisp = new QLabel(this);
    m_clientServerDisp->setText("Клиент: ");
    m_clientServerDisp->setMaximumWidth(50);

    m_comboPaths = new QComboBox(this);
    m_comboPaths->setMaximumSize(80,20);
    m_comboPaths->addItem("user");
    m_comboPaths->addItem("media");
    m_comboPaths->addItem("docs");
    m_comboPaths->addItem("mnt");
    m_comboPaths->addItem("my");
    m_comboPaths->setCurrentIndex(nCurrent);

    m_clientServer = new QComboBox(this);
    m_clientServer->setMaximumSize(80,20);
    m_clientServer->addItem("Клиент");
    m_clientServer->addItem("Сервер");

    m_fileManagerList = new QTreeWidget(this);
    m_fileManagerList->setHeaderLabels(QStringList()<<"Имя"<<"Тип"<<"Размер"<<"Дата");
    m_fileManagerList->setSortingEnabled(false);

    m_isServer = false;

    QHBoxLayout* clientServerLayout = new QHBoxLayout;
    clientServerLayout->addWidget(m_clientServerDisp);
    clientServerLayout->addWidget(m_currentPath);
    clientServerLayout->setSpacing(5);

    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addWidget(m_clientServer);
    hLayout->addWidget(m_comboPaths);
    hLayout->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    hLayout->setSpacing(5);

    QVBoxLayout* mainVLayout = new QVBoxLayout;
    mainVLayout->addLayout(clientServerLayout);
    mainVLayout->addLayout(hLayout);
    mainVLayout->addWidget(m_fileManagerList);
    mainVLayout->setSpacing(5);
    mainVLayout->setMargin(2);

    m_fileManagerList->setColumnWidth(0, 150);
    m_fileManagerList->setColumnWidth(1, 70);
    m_fileManagerList->setColumnWidth(2, 70);
    m_fileManagerList->setColumnWidth(3, 50);

    setLayout(mainVLayout);

    connect(m_clientServer, QOverload<int>::of(&QComboBox::activated), this, &HalfDlg::clientServer);
    connect(m_comboPaths, QOverload<int>::of(&QComboBox::activated), this, &HalfDlg::changeSelectCombo);
    connect(m_fileManagerList, &QTreeWidget::itemClicked, this, &HalfDlg::setActive);
    connect(m_fileManagerList, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item, int column){
        if(!m_isServer) clickDirOrFile(item, column);
        else{
            emit serverTreeItemClicked(m_currentPath->text() + "/" + item->text(column));
        }
    });
}
//---------------------------------------------------------------------------------------------------------------
QString fileSize(long nSize) // Вспомогательная ф-ия перевода размера файла в строку
{
    QString str;
    int i = 0;
    for(i = 0; nSize > 1023; nSize /= 1024, i++) {}
    return str.setNum(nSize) + "BKMGT"[i];
}
//--------------------------------------------------------------------------------------------------------------
void HalfDlg::showDir()
{
    m_currentName = m_currentPath->text();
    m_fileManagerList->clear();
    QDir dir(m_currentName);
    QStringList::Iterator it;
    QFileInfo qFI;
    QTreeWidgetItem *qItem, *qItemFirst = 0;
    // Вверх
    if((m_currentPath->text() != "/home/local/"
        "NPO/a.e.lunkov") && (m_currentPath->text()!="/media")
            && (m_currentPath->text()!="/mnt") && (m_currentPath->text()!="/docs")&& (m_currentPath->text()!="/my")) {
        QStringList pL = QStringList()<<"..."<<"<UP>";
        qItemFirst = new QTreeWidgetItem(m_fileManagerList, pL);
        qItemFirst->setIcon(0, m_imageList[eDIR_UP]);
    }
    // Список каталогов
    QStringList  list = dir.entryList(QDir::Dirs);
    for( it=list.begin();  it != list.end(); ++it )  {
        QCoreApplication::processEvents();
        if( (*it == ".") || (*it == "..") ) continue;  // не берем
        qFI.setFile(dir, *it);
        // Каталог
        QStringList pL = QStringList()<<*it<<"<DIR>"<<""<<qFI.lastModified().toString();
        qItem = new QTreeWidgetItem(m_fileManagerList, pL);  //pkb_fm1
        qItem->setIcon(0, m_imageList[eDIR]);  //pkb_fm1
     }
   // Список файлов
    list = dir.entryList(QDir::Files);
    for( it=list.begin();  it != list.end(); ++it )  {
        QCoreApplication::processEvents();
        if( (*it == ".") || (*it == "..") ) continue;  // не берем
        qFI.setFile(dir, *it);
        QStringList pL = QStringList()<<qFI.completeBaseName()<<qFI.suffix()<<fileSize(qFI.size())<<qFI.lastModified().toString();
        qItem = new QTreeWidgetItem(m_fileManagerList, pL);
        qItem->setIcon(0, m_imageList[getIconFromExt(qFI.suffix())]);
    }

    if(qItemFirst) m_fileManagerList->setCurrentItem(qItemFirst);
    setActive(nullptr, 0);
}
//------------------------------------------------------------------------------------
void HalfDlg::showServerDirs(QJsonObject obj, QStringList catalogs)
{
    QJsonArray jsarrDirs = obj.value("Dirs").toArray();
    QJsonArray jsarrFiles = obj.value("Files").toArray();
    QTreeWidgetItem *qItem, *qItemFirst = 0;

    m_currentName = m_currentPath->text();

    m_fileManagerList->clear();

    if(!catalogs.contains(m_currentPath->text())) {
        QStringList pL = QStringList()<<"..."<<"<UP>";
        qItemFirst = new QTreeWidgetItem(m_fileManagerList, pL);
        qItemFirst->setIcon(0, m_imageList[eDIR_UP]);
    }
    for (int i = 0;i<jsarrDirs.size() ;i++ ) {
        QStringList strlist = jsarrDirs[i].toObject().keys();
        if( (strlist[0] == ".") || (strlist[0] == "..") ) continue;  // не берем
        QStringList pL = QStringList()<<strlist[0]<<"<DIR>"<<""<<jsarrDirs[i].toObject().value(strlist[0]).toString();
        qItem = new QTreeWidgetItem(m_fileManagerList, pL);  //pkb_fm1
        qItem->setIcon(0, m_imageList[eDIR]);  //pkb_fm1
    }
    for (int i = 0;i<jsarrFiles.size() ;i++ ) {
        QJsonObject obj = jsarrFiles[i].toObject();
        if( (obj.value("Name") == ".") || (obj.value("Name") == "..") ) continue;  // не берем
        QStringList pL = QStringList()<<obj.value("Name").toString()<<obj.value("Extension").toString()<<
                                        obj.value("Size").toString()<<obj.value("Data").toString();
        qItem = new QTreeWidgetItem(m_fileManagerList, pL);  //pkb_fm1
        qItem->setIcon(0, m_imageList[getIconFromExt(obj.value("Extension").toString())]);
    }
    if(qItemFirst) m_fileManagerList->setCurrentItem(qItemFirst);
    setActive(nullptr, 0);
}
//-------------------------------------------------------------------------------------
void HalfDlg::switchLabel(bool sw)
{
    if(sw) {
        m_clientServerDisp->setText("Сервер: ");
    }
    else {
        m_clientServerDisp->setText("Клиент: ");
        m_comboPaths->clear();
        m_comboPaths->addItem("user");
        m_comboPaths->addItem("media");
        m_comboPaths->addItem("docs");
        m_comboPaths->addItem("mnt");
        m_comboPaths->addItem("my");
        setCurrentPath("/my");
        showDir();
    }
}
//--------------------------------------------------------------------------------------
void HalfDlg::setComboPaths(QStringList strlist)
{
    m_comboPaths->clear();
    for (int i = 0; i<strlist.size();i++ ) {
        if(strlist[i]!="DialogNum")
        m_comboPaths->addItem(strlist[i]);
    }
    m_comboPaths->setCurrentIndex(0);
}
//----------------------------------------------------------------------------------------------------------------------------
void HalfDlg::changeSelectCombo(int nPos)
{
    if(isServer()){
        switch (nPos) {
        case 0:
            m_currentPath->setText("/home/local/NPO");
            break;
        case 1:
            m_currentPath->setText("/media");
            break;
        case 2:
            m_currentPath->setText("/my");
            break;
    }
        emit serverTreeItemClicked(m_currentPath->text());
    }
    else{
        switch (nPos) {
        case 0:
            m_currentPath->setText("/home/local/NPO/a.e.lunkov");
            break;
        case 1:
            m_currentPath->setText("/media");
            break;
        case 2:
            m_currentPath->setText("/docs");
            break;
        case 3:
            m_currentPath->setText("/mnt");
            break;
        case 4:
            m_currentPath->setText("/my");
            break;
        }
        showDir();
    }
}
//-------------------------------------------------------------------------------------------------------
void HalfDlg::clientServer(int csFlag)
{
    if(csFlag) emit serverCall();   //Выбран сервер
    else emit clientCall();         //Выбран клиент
}
//-------------------------------------------------------------------------------------------------------
void HalfDlg::preOnF3()
{
    QTreeWidgetItem *it = m_fileManagerList->currentItem(); // Выделенный элемент (синий фон)
    if(!it)  it = m_fileManagerList->currentItem(); // Имеющий фокус клавиатуры (пунктирная рамочка)
    clickDirOrFile(it, 0);
}
//-------------------------------------------------------------------------------------------------------
void HalfDlg::clickDirOrFile(QTreeWidgetItem *pItem, int nColumn)  // Кликнули по cписку два раза
{
    if(nColumn || !pItem) return;
    QString sName = pItem->text(0);  // Нулевая колонка (Имя)
    QString sDirOrFile = pItem->text(1);  // Нулевая колонка (Тип)
    if( (sDirOrFile != "<DIR>") && (sDirOrFile != "<UP>") ) {
        // Это файл
        setActive(nullptr, 0);
        onF3(m_currentPath->text() + "/" + sName + "." + pItem->text(1));	// Просмотр файла  // vov_fm
    }
    else {
        // Это каталог
        if( sDirOrFile == "<UP>" ) {
//            // Cтрока выхода на верхний уровень - <UP>
            QDir dir(m_currentPath->text());
            dir.cdUp();
            m_currentPath->setText(dir.absolutePath());
        }
        else  {
            // Строка каталога - <DIR>
            QString strTmp = m_currentPath->text();
            removeEndSlash(strTmp);
            m_currentPath->setText(strTmp + "/" + sName);  // это обычная директория
        }
        showDir();
        //QTimer::singleShot(0, this, SLOT(ShowDir()) );
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------
QString HalfDlg::getFileFromList(const QString operation)
{
    QTreeWidgetItem *it = m_fileManagerList->currentItem(); // Выделенный элемент (синий фон)
    if(!it) {
        QMessageBox::warning(this, tr("Файловый менеджер"), tr("В списке нет выбранного файла"));
        return QString(); // Нет выбранного файла
    }
    QString sName = it->text(0);  // Имя
    QString sType = it->text(1);  // Тип
    if( (sType == "<DIR>") || (sType == "<UP>") ) {
        // Это каталог
        QMessageBox::warning(this, tr("Файловый менеджер"), tr("Невозможно %1 каталог. Выберите файл из списка.").arg(operation));
        return QString();
    }
    if(!checkConfig(sType, operation)) {
        //недопустимое расширение
        QMessageBox::warning(this, tr("Файловый менеджер"), tr("Невозможно %1 файл с расширением .").arg(operation)+sType);
        return QString();
    }
    return (m_currentPath->text() + "/" + sName + "." + it->text(1)); // vov_fm
}
//----------------------------------------------------------------------------------------------------
void HalfDlg::onF3(const QString sFileName) // Просмотр файла
{
    if(sFileName.isNull() || sFileName.isEmpty())  return; // Не удалось определить имя файла
    QString sName = getFileFromList("просмотреть");
    if(sName.isNull() || sName.isEmpty())  return; // Такой файл нельзя смотреть
    m_isFileViewing = sFileName;
    if(!m_logWnd){
        m_logWnd = new LogWindow;
        connect(m_logWnd, &LogWindow::closed, this, &HalfDlg::viewIsOver);
        m_logWnd->show();
        m_logWnd->displayOpenedLog(sName);
    }
    else{
        m_logWnd->clear();
        m_logWnd->show();
        m_logWnd->displayOpenedLog(sName);
    }
}
//----------------------------------------------------------------------------------------------------
void HalfDlg::onF5(FileManager *pFM) // копирование файла
{
    // определим имя файла по выделенной строке в списке
    QString sName = getFileFromList("копировать");
    if(sName.isNull() || sName.isEmpty())  return; // Не удалось определить имя файла
    // или это каталог
    int nPos = sName.lastIndexOf('/'); // vov_fm
    m_isFileCopying = sName;
    if((!this->isServer())&&(!m_otherDlg->isServer()))
        pFM->showCopyDlg(sName, m_otherDlg->getCurrentName() + sName.right(sName.length() - nPos), LocalCopyThread::ClientToClient);
    else if((!this->isServer())&&(m_otherDlg->isServer()))
        pFM->showCopyDlg(sName, m_otherDlg->getCurrentName() + sName.right(sName.length() - nPos), LocalCopyThread::ClientToServer);
    else if((this->isServer())&&(!m_otherDlg->isServer()))
        pFM->showCopyDlg(sName, m_otherDlg->getCurrentName() + sName.right(sName.length() - nPos), LocalCopyThread::ServerToClient);
    else if((this->isServer())&&(m_otherDlg->isServer()))
        pFM->showCopyDlg(sName, m_otherDlg->getCurrentName() + sName.right(sName.length() - nPos), LocalCopyThread::ServerToServer);
    return;
}
//----------------------------------------------------------------------------------------------------
#include <QTextCodec>
int HalfDlg::onF8()  //удаление файла
{
    // определим имя файла по выделенной строке в списке
    QString sFileName = getFileFromList("удалить");
    if(sFileName.isNull() || sFileName.isEmpty())  return 0; // Не удалось определить имя файла
    // или это каталог
    //Реализовать проверку на воспроизведение удаляемого файла
    if (m_isFileViewing == sFileName){
        QMessageBox::warning(this, tr("Ошибка удаления"), tr("Файл невозможно удалить, так как он в данный момент просматривается"));
        return 0;
    }
    if (m_isFileCopying == sFileName){
        QMessageBox::warning(this, tr("Ошибка удаления"), tr("Файл невозможно удалить, так как он в данный момент копируется"));
        return 0;
    }
    if(QMessageBox::question(this, tr("Файловый менеджер"),"Удалить файл " + sFileName + "?",
                             QMessageBox::Yes, QMessageBox::No )==QMessageBox::No) return 0;

    if(m_isServer)
        emit deleteFile(sFileName);
    else
        deleteFileAsync(sFileName);
    return 1;
}
//---------------------------------------------------------------------------------------------------------
//char sFileExtR[10];//pkb_ext
void HalfDlg::onF9(FileManager *pFM) // Вырезание файла (Recreate)
{
        QString sFileName = getFileFromList("вырезать");
//    // определим имя файла по выделенной строке в списке
//    QString sName = GetFileFromList("редактировать");
//    if(sName.isNull() || sName.isEmpty())  return; // Не удалось определить имя файла
//    // или это каталог
//    int nPos = sName.lastIndexOf(".");
//    QString sExt = "";
//    // Считываем расширение
//    if(nPos > 0)  sExt = sName.right(sName.length() - nPos - 1);
//    strcpy(sFileExtR, sExt.toStdString().c_str());//pkb_ext
//    // Если не kt, не te, не kdp - выходим
//    if((sExt != "kt") && (sExt != "te") && (sExt != "kdp")) {
//        MessageBoxQt("Возможно редактировать только файлы с расширениями ""kt"", ""te"", ""kdp""","Ошибка редактирования");
//        return;
//    }
//    pFM->ShowRecreateDlg(sName);
//    return;
}
//------------------------------------------------------------
int HalfDlg::getIconFromExt(QString sExt)
{
    if((sExt == "log") || (sExt == "deb"))  return eLOG;
    if(sExt == "kt")  return eKT;
    if((sExt == "te") || (sExt == "kdp"))  return  eTE;
    return eDEF;
}
//-----------------------------------------------------------------
bool HalfDlg::checkConfig(const QString fExt, const QString operation)
{
    QJsonObject extensions;

    QJsonArray viewing;
    viewing.push_back("log");
    //viewing.push_back("ini");
    extensions.insert("просмотреть", viewing);

    QJsonArray copying;
    copying.push_back("kt");
    copying.push_back("log");
    copying.push_back("ini");
    copying.push_back("odg");
    extensions.insert("копировать", copying);

    QJsonArray deleting;
    deleting.push_back("kt");
    deleting.push_back("log");
    deleting.push_back("ini");
    extensions.insert("удалить", deleting);

    QJsonArray cutting;
    cutting.push_back("kt");
    extensions.insert("вырезать", cutting);

    QJsonDocument doc(extensions);

//    QSettings* set = new QSettings("/home/local/NPO/a.e.lunkov/Config/my_config.ini", QSettings::IniFormat);
//    set->beginGroup("extensions");
//    QStringList list;
//    QByteArray barr;
//    list<< "kt" << "log" << "odg";
//    set->setValue("ext", list);
//    set->sync();
//    list = set->value("ext").toStringList();

//    if(list.contains(fExt)) return 1;
        if(extensions.value(operation).toArray().contains(fExt)) return 1;
    else return 0;
}
//========================================================================


