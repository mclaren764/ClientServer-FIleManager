//#include "mainwindow.h"
#include "MainIko.h"
#include "AskuWindow.h"
#include "Version.h"
#include "CurrentTime.h"
#include "Loger.h"
#include "SettingsBase.h"
#include "ParserTypes.h"
#include "NetThread.h"
#include "ServersDlg.h"
#include "UserDlg.h"
#include "VoiTypes.h"
#include "FileManager.h"

#include <QApplication>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QThread>
#include <QJsonDocument>

#include <signal.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <QDebug>

#include <Common/LoggerMessageHandler.h>
#include <Common/UI/UI_Utils.h>

#include "RlkContainer.h"

#define sLogPrefix   "Main"

uint n__SerialNumber = 0;
//---------------------------------------------------------
static const char HELP_MESSAGE[] =
        "-h, --help            Вывод справки о параметрах запуска\n"
        "-v, --version         Вывод версии ПО\n"
//        "--configfile <file>   Использовать файл <file> для инициализации параметров, по умолчанию используется \"IniData.ini\"\n"
        "--documdir <dir>      Использовать каталог <dir> для документирования информации, по умолчанию используется \"/opt/lemz/rlk/voi/DocumVOI\"\n";
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
void ParseArgums(int argc, char *argv[], QString &sDocumDir)
{
  if(argc > 1) {
    for(int k = 1; k < argc; k++) { // цикл по аргументам
      std::string sKey = argv[k];
      if((sKey == "-v") || (sKey == "--version")) {
        std::cout<<"РЛК-МЦ вторичная обработка информации (клиент) версия программы "<<QCoreApplication::applicationVersion().toStdString()<<std::endl;
        exit(EXIT_SUCCESS);
        }

      // Если задано имя каталога для документирования, то используем его
      if(sKey == "--documdir") {
        if((k + 1) < argc) sDocumDir = argv[k + 1];
        else MY_LOG_ERR(QObject::tr("Некорректные параметры запуска, отсутсвует имя папки для документирования"));
        }
      // вывод справки
      if((sKey == "-h") || (sKey == "--help")) {
        std::cout << HELP_MESSAGE << std::endl;
        std::cout.flush();
        exit(EXIT_SUCCESS);
        }
      } // цикл по аргументам
    }
}
//---------------------------------------------------------
int main(int argc, char *argv[])
{
    // Настраиваем свой обработчик для функций логирования Qt
    LoggerMessageHandler::init();

    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("lemz");
    QCoreApplication::setOrganizationDomain("www.lemz.ru");
    QCoreApplication::setApplicationName("RLK_Voi_Client");

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

    // создадим двух пользователей по умолчанию:
    // 1. admin - adminadmin с правами администратора, может создавать других пользователей
    // 2. operator - operator с правами оператора

    QString sDocumDir;
    ParseArgums(argc, argv, sDocumDir);

    OUserDlg* pUserDlg = new OUserDlg;
   // if(!pUserDlg || (pUserDlg->exec() != QDialog::Accepted) ) {
   //   delete pUserDlg;
   //   exit(EXIT_FAILURE);
   //   }

    // папка с настройками для каждого оператора своя
   // QString sConfFileUse = pUserDlg->User()->s_Name;
     QString sConfFileUse = "admin";
     SettingsBase oSet;
     oSet.setDirName(sConfFileUse);

    // папка для документирования
    QString sDocumDirDefault("home/Projects/voi_client_server"), sDocumDirUse;
    if(!sDocumDir.isEmpty()) {
      QFileInfo oInfo(sDocumDir);
      if(!oInfo.exists() || !oInfo.isDir()) {
        if(QDir("").mkpath(sDocumDir)) sDocumDirUse = sDocumDir;
        else {
          MY_LOG_ERR(QObject::tr("Невозможно создать каталог для документирования: \"%1\"").arg(sDocumDir));
          }
        }
      else sDocumDirUse = sDocumDir;
      }

   // SettingsBase oSet;
    QString sKeyDocumPath("DocumPath");
    if(sDocumDirUse.isEmpty()) {
      // смотрим что в конфиге
      sDocumDir = oSet.value(sKeyDocumPath, sDocumDirDefault).toString();
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
    QString sKTName("FileKT_Cl");
    QString sLogName = QString("FileLog_Cl");
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
    oSet.setValue(sKeyDocumPath, sDocumDirUse);
    sLogName = sDocumDirUse + sLogName + "/" + QDateTime::currentDateTimeUtc().toString(sDateFormat) + ".log";
    oLogFile.SetLogFile(sLogName);

    MY_LOG_INFO(QObject::tr(""));
    MY_LOG_INFO(QObject::tr(""));
    MY_LOG_INFO(QObject::tr(""));
    MY_LOG_INFO(QObject::tr("========================================================================="));
    MY_LOG_INFO(QObject::tr("========================================================================="));
    MY_LOG_INFO(QObject::tr("========================================================================="));
    MY_LOG_INFO(QObject::tr("===== Старт ПО вторичной обработки (клиент) для изделия \"РЛК-МЦ\""));
    MY_LOG_INFO(QObject::tr("===== Версия ПО: %1").arg(QCoreApplication::applicationVersion()));
    MY_LOG_INFO(QObject::tr("===== pid: %1").arg(((pid_t)syscall(SYS_gettid))));
    MY_LOG_INFO(QObject::tr("Имя исполняемого файла %1").arg(QFileInfo(sProgrammFile).absoluteFilePath()));
    MY_LOG_INFO(QObject::tr("Имя каталога для документирования %1").arg(sDocumDirUse));
    MY_LOG_INFO(QObject::tr("Имя файла с настройками %1").arg(sConfFileUse));

    // создаем потоки
    QThread *pNetThread = new QThread;
    NetObject *pNet = new NetObject(pNetThread, sDocumDirUse + sKTName);

    OMainIKOWnd w(pUserDlg, nullptr);
    w.ShowServersDlg();
    RlkContainer::get().setIko(w.getIko());
    // коннекты нужно куда-то убрать
    qRegisterMetaType<MultiAnswer>("MultiAnswer");
    QObject::connect(pNet, &NetObject::SigMultiAns, w.ServersWnd(), &OServersWnd::AddMultiAns);
    QObject::connect(pNet, &NetObject::SigStatusToServerConnection, w.ServersWnd(), &OServersWnd::StatusToServerConnection);
    QObject::connect(w.ServersWnd(), &OServersWnd::SigConnectToServer, pNet, &NetObject::ConnectToServer);
    QObject::connect(pNet, &NetObject::SigStatusToServerConnection, w.getIko(), &MasshIKO::updateServerStatus);

    FileManager *fmwnd = new FileManager(w.getIko());
    fmwnd->show();

    qRegisterMetaType<HeaderModuleData>("HeaderModuleData");
    qRegisterMetaType<SendModuleDataToFromRmo>("SendModuleDataToFromRmo");
    qRegisterMetaType<SendRlkDataToRmo>("SendRlkDataToRmo");
    //QObject::connect(pNet, &NetObject::SigModulData, &w, &OMainIKOWnd::RecvModulData);
    QObject::connect(pNet, &NetObject::moduleDataReceived,
                     &RlkContainer::get(), &RlkContainer::onModuleDataReceived);
    QObject::connect(pNet, &NetObject::rlkDataReceived,
                     &RlkContainer::get(), &RlkContainer::onRlkDataReceived);
    QObject::connect(pNet, &NetObject::unionTrackReceived,
                     w.getIko(), &MasshIKO::onUnionTrackReceived);
    QObject::connect(pNet, &NetObject::stepReceived,
                     w.getIko(), &MasshIKO::onStepReceived);
    QObject::connect(w.getIko(), &MasshIKO::nextStepSended,
                     pNet, &NetObject::nextStepSended);
    QObject::connect(fmwnd, &FileManager::FMDataSended, pNet, &NetObject::FMDataSended);
    QObject::connect(pNet, &NetObject::FMDataRecieved, fmwnd, &FileManager::recieveFMData);



    auto askuWindow = new AskuWindow();
    QObject::connect(pNet, &NetObject::schemesReceived, askuWindow, &AskuWindow::schemesRecvd);
    QObject::connect(pNet, &NetObject::elemDatasReceived, askuWindow, &AskuWindow::elemsRecvd);
    QObject::connect(pNet, &NetObject::ctrlParamDatasReceived, askuWindow, &AskuWindow::ctrlParamsRecvd);
    QObject::connect(pNet, &NetObject::moduleStatusesReceived, askuWindow, &AskuWindow::moduleStatsRecvd);
    QObject::connect(pNet, &NetObject::tuneParamDatasReceived, askuWindow, &AskuWindow::tuneParamsRecvd);
    QObject::connect(askuWindow, &AskuWindow::newTuneParamValues, pNet, &NetObject::newTuneParamValues);

    w.show();
    askuWindow->show();

    pNetThread->start();
    int nRet =  a.exec();

    pNet->Close();
    pNet->deleteLater();
    pNetThread->deleteLater();
    askuWindow->deleteLater();

    MY_LOG_INFO(QObject::tr("Работа ВОИ-клиент завершена с кодом %1").arg(nRet));
    return nRet;
}
