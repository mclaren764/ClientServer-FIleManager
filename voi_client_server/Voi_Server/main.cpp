#include <QCoreApplication>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QTimer>
#include <QDataStream>

#include "Enums.h"
#include "Loger.h"
#include "Version.h"
#include "CurrentTime.h"
#include "SettingsBase.h"
#include "ThreadController.h"
#include "TypesFunction.h"

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <Common/LoggerMessageHandler.h>
#include "Conversions.h"

#define sLogPrefix   "Main"

uint n__SerialNumber = 0;

//---------------------------------------------------------
static const char HELP_MESSAGE[] =
        "-h, --help            Вывод справки о параметрах запуска\n"
        "-v, --version         Вывод версии ПО\n"
        "--daemon              Запуск программы-\"демона\"\n"
        "--workmode <mode>     Режим работы: \"br\" - штатная работа (по умолчанию), \"rd\" - режим чтения файлов\n"
        "--configdir <dir>     Использовать каталог <dir> для инициализации параметров, по умолчанию используется \"./IniData\"\n"
        "--documdir <dir>      Использовать каталог <dir> для документирования информации, по умолчанию используется \"/opt/lemz/rlk/voi/DocumVOI\"\n"
        "--pidfile <file>      Использовать <file> для записи PID (имя без расширения), по умолчанию используется \"./VOI_Server_1(2).pid\"\n";

//        "--debug             Print debug messages\n"
//        "--logfile <file>    Использовать <file> для записи сообщений, по умолчанию используется \"./VOI_Server.log\"\n"
//        "                    If use \"daemon\": first set \"workdir\", after this open \"logfile\"\n"
//        "--workdir <dir>     Использовать <dir> в качестве рабочего каталога, по умолчанию используется \"./\"\n"
//        "                    Игнорируется, если не установлен флаг \"daemon\"\n";
//---------------------------------------------------------
void term_handler(int i, siginfo_t *si, void *)
{
    MY_LOG_WARN(QObject::tr("Получен сигнал %1").arg(i));

    std::string sNameFileProc = (QObject::tr("/proc/%1/comm").arg((int)si->si_pid)).toStdString();
    std::string sNameSender = std::string("UNKNOWN");
    if (QFile(sNameFileProc.c_str()).exists())  {
      try {
        std::ifstream flNameProc(sNameFileProc.c_str());
        flNameProc >> sNameSender;
        flNameProc.close();
        }
      catch(...) {
        MY_LOG_WARN("Невозможно получить имя отправителя");
        }
      }
    MY_LOG_WARN(QObject::tr("Отправитель сигнала: pid: %1, имя: %2, UID: %3").arg(si->si_pid).arg(sNameSender.c_str()).arg(si->si_uid));

    QCoreApplication::exit();
}
//---------------------------------------------------------
int CheckRuning(QString &sPidFileName)
{
    const QString sExt(".pid");
    QString sFileName1 = sPidFileName + "_1" + sExt;

    if(!QFile(sFileName1).exists()) {
      if(!QFile(sFileName1).open(QIODevice::ReadWrite)) {
        MY_LOG_ERR(QObject::tr("Задан несуществующий pid-файл \"%1\"").arg(sFileName1));
        MY_LOG_ERR(QObject::tr("Ошибка при попытке создания pid-файла"));
        exit(EXIT_FAILURE);
        }
      }

    QFileInfo oInfo1(sFileName1);
    if((!oInfo1.exists()) || (!oInfo1.isReadable()) || (oInfo1.size() == 0)) {
      sPidFileName = sFileName1;
      return 0; // ничего не запущено
      }

    std::ifstream flPid1(sFileName1.toStdString().c_str());
    uint pidCheck;
    flPid1 >> pidCheck;
    flPid1.close();

    int isRunRes = kill(pidCheck, 0);
    if (isRunRes == -1) {
      try {
        std::ofstream flPid(sFileName1.toStdString().c_str(), std::ios_base::trunc);
        flPid.close();
        sPidFileName = sFileName1;
        return 0; // ничего не запущено
        }
      catch(...) {
        MY_LOG_ERR(QObject::tr("Ошибка при освобождении PID-файла: \"%1\"").arg(sFileName1));
        exit(EXIT_FAILURE);
        }
      }
    else { // первое приложение уже запущено, смотрим, есть ли второе
      // второе может быть запущено только в режиме чтения
      QString sFileName2 = sPidFileName + "_2" + sExt;

      if(!QFile(sFileName2).exists()) {
        if(!QFile(sFileName2).open(QIODevice::ReadWrite)) {
          MY_LOG_ERR(QObject::tr("Задан несуществующий pid-файл \"%1\"").arg(sFileName2));
          MY_LOG_ERR(QObject::tr("Ошибка при попытке создания pid-файла"));
          exit(EXIT_FAILURE);
          }
        }

      QFileInfo oInfo2(sFileName2);
      if((!oInfo2.exists()) || (!oInfo2.isReadable()) || (oInfo2.size() == 0)) {
        sPidFileName = sFileName2;
        return 1; // запущено одно приложение
        }
      std::ifstream flPid2(sFileName2.toStdString().c_str());
      flPid2 >> pidCheck;
      flPid2.close();

      int isRunRes = kill(pidCheck, 0);
      if (isRunRes == -1) {
        try {
          std::ofstream flPid(sFileName2.toStdString().c_str(), std::ios_base::trunc);
          flPid.close();
          sPidFileName = sFileName2;
          return 1;
          }
        catch(...) {
          MY_LOG_ERR(QObject::tr("Ошибка при освобождении PID-файла: \"%1\"").arg(sFileName2));
          exit(EXIT_FAILURE);
          }
        }
      else {
        MY_LOG_ERR(QObject::tr("Уже запущено два процесса, запуск третьего не возможен"));
        exit(EXIT_FAILURE);
        }
      } // первое приложение уже запущено, смотрим, есть ли второе
    //return 0;
}
//---------------------------------------------------------
int RunAsDaemon()
{
    pid_t pid, sid;
    MY_LOG_INFO(QObject::tr("Запуск программы в качестве демона"));

    // Ответвляемся от родительского процесса
    pid = fork();
    if (pid < 0) {
      MY_LOG_ERR(QObject::tr("PID: %1: Не создался дочерний процесс").arg(pid));
      exit(EXIT_FAILURE);
      }
    // Если с PID'ом все получилось, то родительский процесс можно завершить.
    if (pid > 0) exit(EXIT_SUCCESS);

    // Изменяем файловую маску
    umask(0);
    // Создание нового SID для дочернего процесса
    sid = setsid();
    if (sid < 0) {
      MY_LOG_ERR("Невозможно задать SID для процесса");
      exit(EXIT_FAILURE);
      }

    MY_LOG_INFO(QObject::tr("\"Демон\" успешно запущен. Новый PID: %1").arg(pid));
    return 0;
}
//---------------------------------------------------------
int UpdatePidFile(const QString &sPidFileName)
{
    try {
      std::ofstream flPid(sPidFileName.toStdString().c_str(), std::ios_base::trunc);
      flPid << getpid();
      flPid.flush();
      flPid.close();
      }
    catch(...) {
      MY_LOG_ERR(QObject::tr("Ошибка при попытке записи PID-а в файл: \"%1\"").arg(sPidFileName));
      exit(EXIT_FAILURE);
      }

    MY_LOG_INFO(QObject::tr("PID-файл \"%1\" обновлен").arg(sPidFileName));
    return 0;
}
//---------------------------------------------------------
int ClearPidFile(const QString &sPidFileName)
{
    MY_LOG_INFO(QObject::tr("Очистка PID-файла \"%1\"").arg(sPidFileName));
    try {
      std::ofstream flPid(sPidFileName.toStdString().c_str(), std::ios_base::trunc);
      flPid.close();
      }
    catch(...)  {
      MY_LOG_ERR(QObject::tr("Ошибка при попытке очистить PID-файл: \"%1\"").arg(sPidFileName));
      exit(EXIT_FAILURE);
      }
    return 0;
}
//---------------------------------------------------------
void ParseArgums(int argc, char *argv[], QString &sConfigDir, bool &bIsDaemon, QString &sDocumDir,
                 QString &sPidFile, QString &sWkMode)
{
  if(argc < 2) return;
  for(int k = 1; k < argc; k++) { // цикл по аргументам
    std::string sKey = argv[k];
    if((sKey == "-v") || (sKey == "--version")) {
      std::cout<<"РЛК-МЦ вторичная обработка информации (сервер) версия программы "<<QCoreApplication::applicationVersion().toStdString()<<std::endl;
      exit(EXIT_SUCCESS);
      }

    // Если задано имя папки с конфигами, то используем ее
    if(sKey == "--configdir") {
      if((k + 1) < argc) sConfigDir = argv[k + 1];
      else MY_LOG_ERR(QObject::tr("Некорректные параметры запуска, отсутствует имя папки с параметрами инициализации"));
      }
    // Если задан флаг запуска в режиме демона
    if(sKey == "--daemon") bIsDaemon = true;
    // Если задано имя pid-файла, то используем его
    if(sKey == "--pidfile") {
      if((k + 1) < argc) sPidFile = argv[k + 1];
      else MY_LOG_ERR(QObject::tr("Некорректные параметры запуска, отсутствует имя Pid-файла"));
      }
    // Если задано имя каталога для документирования, то используем его
    if(sKey == "--documdir") {
      if((k + 1) < argc) sDocumDir = argv[k + 1];
      else MY_LOG_ERR(QObject::tr("Некорректные параметры запуска, отсутсвует имя папки для документирования"));
      }
    // Если задан режим работы, то используем его
    if(sKey == "--workmode") {
      if((k + 1) < argc) sWkMode = argv[k + 1];
      else MY_LOG_ERR(QObject::tr("Некорректные параметры запуска, отсутсвует режим работы программы"));
      }
    // вывод справки
    if((sKey == "-h") || (sKey == "--help")) {
      std::cout << HELP_MESSAGE << std::endl;
      std::cout.flush();
      exit(EXIT_SUCCESS);
      }
    } // цикл по аргументам
}
//---------------------------------------------------------
int main(int argc, char *argv[])
{
    // Настраиваем свой обработчик для функций логирования Qt
    LoggerMessageHandler::init();

    QCoreApplication::setSetuidAllowed(true);
    QCoreApplication a(argc, argv);

    QCoreApplication::setOrganizationName("lemz");
    QCoreApplication::setOrganizationDomain("www.lemz.ru");
    QCoreApplication::setApplicationName("RLK_Voi_Server");

    QString sProgrammFile = argv[0];
    QFileInfo oInfo(sProgrammFile);
    if(oInfo.isSymLink()) {
      sProgrammFile = oInfo.symLinkTarget();
      oInfo.setFile(sProgrammFile);
      }

    QString sTime = oInfo.birthTime().toString(sDateTimeFormat);
    QCoreApplication::setApplicationVersion(QString("%1.%2 (%3)").arg(n__MajorVerPO).arg(n__MinorVerPO).arg(sTime));

    struct sigaction sa1, sa2, sa3;
    sa1.sa_flags = SA_SIGINFO;
    sa2.sa_flags = SA_SIGINFO;
    sa3.sa_flags = SA_SIGINFO;
    sigemptyset(&sa1.sa_mask);
    sigemptyset(&sa2.sa_mask);
    sigemptyset(&sa3.sa_mask);
    sa1.sa_sigaction = term_handler;
    sa2.sa_sigaction = term_handler;
    sa3.sa_sigaction = term_handler;

    sigaction(SIGTERM, &sa1, 0);
    sigaction(SIGABRT, &sa2, 0);
    sigaction(SIGINT, &sa3, 0);

    // если отсутствует файл с серийным номером, то выходим
    QFile oFSN(s__SerialNumberFile);
    if(!oFSN.exists()) {
      MY_LOG_ERR(QObject::tr("Отсутствует файл с серийным номером изделия, запуск программы невозможен"));
      exit(EXIT_FAILURE);
      }
    // считываем серийный номер
    if(!oFSN.open(QIODevice::ReadOnly)) {
      MY_LOG_ERR(QObject::tr("Не открывается файл с серийным номером изделия, запуск программы невозможен"));
      exit(EXIT_FAILURE);
      }
    QDataStream in(&oFSN);
    while(!in.atEnd()) {
      in>>n__SerialNumber;
      }
    oFSN.close();

    QString sConfDir, sDocumDir, sPidFile, sWkMode;
    bool bIsDaemon = false;
    ParseArgums(argc, argv, sConfDir, bIsDaemon, sDocumDir, sPidFile, sWkMode);

    if(sPidFile.isEmpty()) sPidFile = "./VOI_Server";
    else {
      QFileInfo oInfo(sPidFile);
      if(!oInfo.suffix().isEmpty()) sPidFile = sPidFile.left(sPidFile.size() - oInfo.suffix().size() + 1);
      }
    uint nWkMode = eBRMode;
    if(!sWkMode.isEmpty()) {
      if(sWkMode.compare("br", Qt::CaseInsensitive) == 0)  nWkMode = eBRMode;
      else if(sWkMode.compare("rd", Qt::CaseInsensitive) == 0)  nWkMode = eRDMode;
      else MY_LOG_ERR(QObject::tr("Задан несуществующий режим работы программы: \"%1\", "
                                  "запуск в режиме штатной работы").arg(sWkMode));
      }
    int nCnt = CheckRuning(sPidFile);
    if(nCnt && nWkMode == eBRMode) { // уже запущено одно приложение и требуется БР
      MY_LOG_ERR(QObject::tr("Запуск второго приложения в штатном режиме работы невозможен"));
      exit(EXIT_FAILURE);
      }
    SettingsBase set;
    // файлы инициализации
    QString sConfDirUse;
    if(!sConfDir.isEmpty()) {
      QFileInfo oInfo(sConfDir);
      if(!oInfo.exists() || !oInfo.isDir() || !oInfo.isReadable() || !oInfo.isWritable()) {
        MY_LOG_ERR(QObject::tr("Задан несуществующий каталог с параметрами инициализации: \"%1\"").arg(sConfDir));
        }
      else sConfDirUse = sConfDir;
      }
    if(sConfDirUse.isEmpty()) {
      sConfDirUse = "./IniData";
      QFileInfo oInfo(sConfDirUse);
      if(!oInfo.exists() || !oInfo.isDir()) {
        if(!QDir::current().mkdir(sConfDirUse) ) {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог с параметрами инициализации: \"%1\"").arg(sConfDirUse));
          if(nWkMode == eBRMode) exit(EXIT_FAILURE);
          sConfDirUse = "";
          }
        }      
      set.setDirName(sConfDirUse);
      if(!set.isWritable()) {
        MY_LOG_ERR(QObject::tr("Невозможно создать файл с параметрами инициализации: \"%1\"").arg(set.fullFileName()));
        if(nWkMode == eBRMode) exit(EXIT_FAILURE);
        }
      }

    // папка для документирования
    QString sDocumDirDefault("home/Projects/voi_client_server"), sDocumDirUse;
    if(!sDocumDir.isEmpty()) {
      QFileInfo oInfo(sDocumDir);
      if(!oInfo.exists() || !oInfo.isDir()) {
        if(QDir("").mkpath(sDocumDir)) sDocumDirUse = sDocumDir;
        else MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sDocumDir));
        }
      else sDocumDirUse = sDocumDir;
      }

    QString sKeyDocumPath = (nWkMode == eBRMode) ? "DocumPath" : "DocumPath_Rd";
    if(sDocumDirUse.isEmpty()) {
      // смотрим что в конфиге
      sDocumDir = set.value(sKeyDocumPath, sDocumDirDefault).toString();
      QFileInfo oInfo(sDocumDir);
      if(!oInfo.exists() || !oInfo.isDir()) {
        if(!QDir("").mkpath(sDocumDir)) {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sDocumDir));
          if(sDocumDir == sDocumDirDefault) exit(EXIT_FAILURE);
          }
        else sDocumDirUse = sDocumDir;
        }
      else sDocumDirUse = sDocumDir;
      }
    if(sDocumDirUse.isEmpty()) {
      sDocumDirUse = sDocumDirDefault;
      QFileInfo oInfo(sDocumDirUse);
      if(!oInfo.exists() || !oInfo.isDir()) {
        if(!QDir("").mkpath(sDocumDirUse)) {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sDocumDirUse));
          exit(EXIT_FAILURE);
          }
        }
      }
    if(sDocumDirUse.at(sDocumDirUse.size() - 1) != '/') sDocumDirUse.append("/");
    QDir oDocumDir(sDocumDirUse);
    QString sLogName;
    QString sKTName("FileKT");
    if(nWkMode == eBRMode) {
      sWkMode = "штатной работы";
      sLogName = QString("FileLog");
      if(!QDir(sDocumDirUse + sLogName).exists()) {
        if(!oDocumDir.mkdir(sLogName)) {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sLogName));
          exit(EXIT_FAILURE);
          }
        }

      if(!QDir(sDocumDirUse + sKTName).exists()) {
        if(!oDocumDir.mkdir(sKTName)) {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sKTName));
          exit(EXIT_FAILURE);
          }
        }
      QString sKTOldName("OldFileKT");
      if(!QDir(sDocumDirUse + sKTOldName).exists()) {
        if(!oDocumDir.mkdir(sKTOldName)) {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sKTOldName));
          exit(EXIT_FAILURE);
          }
        }
      QString sOutName("FileOut");
      if(!QDir(sDocumDirUse + sOutName).exists()) {
        if(!oDocumDir.mkdir(sOutName)) {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sOutName));
          exit(EXIT_FAILURE);
          }
        }
      }
    else if(nWkMode == eRDMode) {
      sWkMode = "чтения файла";
      sLogName = QString("FileLog_Rd_%1").arg(nCnt + 1);
      if(!QDir(sDocumDirUse + sLogName).exists()) {
        if(!oDocumDir.mkdir(sLogName)) {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sLogName));
          exit(EXIT_FAILURE);
          }
        }
      }
    set.setValue(sKeyDocumPath, sDocumDirUse);

    set.setReadOnly(IsNoBrMode(nWkMode)); // запрет на запись конфигов в режиме чтения
    sLogName = sDocumDirUse + sLogName + "/" + QDateTime::currentDateTimeUtc().toString(sDateFormat) + ".log";
    oLogFile.SetLogFile(sLogName);
    if(bIsDaemon)  {
      MY_LOG_INFO(QObject::tr("Запуск в качестве \"Демона\""));
      RunAsDaemon();
      }
    UpdatePidFile(sPidFile);

    MY_LOG_INFO(QObject::tr("========================================================================="));
    MY_LOG_INFO(QObject::tr("========================================================================="));
    MY_LOG_INFO(QObject::tr("========================================================================="));
    MY_LOG_INFO(QObject::tr("Старт ПО вторичной обработки для изделия \"РЛК-МЦ\" в режиме %1").arg(sWkMode));
    MY_LOG_INFO(QObject::tr("Версия ПО: %1").arg(QCoreApplication::applicationVersion()));
    MY_LOG_INFO(QObject::tr("pid: %1").arg(((pid_t)syscall(SYS_gettid))));
    MY_LOG_INFO(QObject::tr("Имя исполняемого файла %1").arg(QFileInfo(sProgrammFile).absoluteFilePath()));
    MY_LOG_INFO(QObject::tr("Имя каталога для документирования %1").arg(sDocumDirUse));
    MY_LOG_INFO(QObject::tr("Имя каталога с настройками %1").arg(sConfDirUse));
    MY_LOG_INFO(QObject::tr("Имя Pid-файла %1").arg(sPidFile));

    // создаем и запускаем все потоки
    ThreadController threadController(nWkMode, nCnt, sDocumDirUse + sKTName);

    QTimer oTestTimer;
    oTestTimer.setInterval(300);
    QObject::connect(&oTestTimer, &QTimer::timeout, &threadController, &ThreadController::onTestTreads);
    oTestTimer.start();
    int nRet = a.exec();

    oTestTimer.stop();
    // вставить запись о статусе комплекса: РАФ, РГДВ, ...
    ClearPidFile(sPidFile);
    MY_LOG_INFO(QObject::tr("Работа ВОИ-сервер завершена с кодом %1").arg(nRet));
    return nRet;
}

