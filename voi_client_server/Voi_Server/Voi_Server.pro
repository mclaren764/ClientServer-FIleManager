
#DEPENDPATH += ../Voi_Common
#INCLUDEPATH += ../Voi_Common

include($$PWD/../Voi_Common/Voi_Common.pri)
include($$PWD/../Common/Common.pri)


#QT -= gui
CONFIG -= app_bundle


TEMPLATE = app

DESTDIR = ../bin

CONFIG(debug, debug|release) {
  TARGET = Voi_Server_d
}
else {
  TARGET = Voi_Server
}

CONFIG += console
CONFIG -= app_bundle

INCLUDEPATH += Parsers Modules Tracks AsterixTypes AsterixCategory \
              ServerSockets SimulatedModules
DEPENDPATH += Parsers Modules Tracks AsterixTypes AsterixCategory \
              ServerSockets SimulatedModules

include(AskuComplex/AskuComplex.pri)

DEFINES += __MY_OPENCV__
contains(DEFINES, __MY_OPENCV__) {
    LIBS += -lopencv_core -lopencv_highgui
    HEADERS +=

    SOURCES +=

    DEFINES += __MY_FFMPEG__
    contains(DEFINES, __MY_FFMPEG__) {
       LIBS += -lavformat \
            -lavcodec \
            -lavutil \
            -lswscale \
            -lswresample
     }
 }


SOURCES += main.cpp \
    FMObject.cpp \
    Settings.cpp \
    RlkServer.cpp \
    CreateModuleObj.cpp \
    ReadObj.cpp \
    NetObj.cpp \
    ThreadController.cpp \
    VoiObj.cpp \
    TypeVersionForCreate.cpp \
    Fft_calc.cpp \
    Fuzzificator.cpp \
    FiToH.cpp \
    Parsers/ParserBla.cpp \
    Parsers/ParserKoir.cpp \
    Parsers/ParserMkc.cpp \
    Parsers/ParserPoi.cpp \
    Parsers/ParserReb.cpp \
    Parsers/ParserRequest.cpp \
    Parsers/ParserRmo.cpp \
    Parsers/ParserRtr.cpp \
    Parsers/ParserServerBase.cpp \
    Parsers/ParserSpp.cpp \
    Parsers/ParserVoiKt.cpp \
    Modules/ModuleServer.cpp \
    Modules/ModuleServerBase.cpp \
    Modules/ModuleServerKoir.cpp \
    Modules/ModuleServerMkc.cpp \
    Modules/ModuleServerBla.cpp \
    Modules/ModuleServerPoi.cpp \
    Modules/ModuleServerMkcFuzz.cpp \
    Tracks/TrackBuf.cpp \
    Tracks/TrackSourceBase.cpp \
    Tracks/TrackKoir.cpp \
    Tracks/TrackBla.cpp \
    AzTimePoi.cpp \
    Filtrs/SingleLineFiltr.cpp \
    Tracks/TrackRls.cpp \
    Tracks/TrackUnion.cpp \
    Modules/ModuleServerRmo.cpp \
    Parsers/ParserAsterixBase.cpp \
    Parsers/ParserAsterixAzn.cpp \
    AsterixCategory/AsterixCategory021.cpp \
    AsterixCategory/AsterixCategory023.cpp \
    AsterixCategory/AsterixCategoryItem.cpp \
    AsterixTypes/AsterixAzn023.cpp \
    AsterixTypes/AsterixAzn021.cpp \
    AsterixTypes/AsterixVer247.cpp \
    AsterixCategory/AsterixCategory247.cpp \
    ServerSockets/SocketTcpServer.cpp \
    ServerSockets/SocketUdpAsterix.cpp \
    SimulatedModules/ModuleSimulatedBase.cpp \
    Parsers/ParserClientAzn.cpp \
    SimulatedModules/ModuleSimulatedAzn.cpp \
    AsterixTypes/AsterixSm034.cpp \
    AsterixCategory/AsterixCategory034.cpp \
    Parsers/ParserAsterixSva.cpp \
    AsterixTypes/AsterixState253.cpp \
    AsterixCategory/AsterixCategory253.cpp \
    AsterixTypes/AsterixState253Header.cpp \
    SimulatedModules/ModuleSimulatedSva.cpp \
    AsterixTypes/AsterixTrack048.cpp \
    AsterixTypes/AsterixTrack048Header.cpp \
    AsterixCategory/AsterixCategory048.cpp \
    SimulatedModules/ModuleSimulatedAsterix.cpp \
    Parsers/ParserClientSva.cpp \
    Parsers/ParserSva.cpp \
    Modules/ModuleServerSva.cpp \
    SimulatedSvaWorkObject.cpp \
    SimulatedAznWorkObject.cpp \
    Parsers/ParserAzn.cpp \
    Modules/ModuleServerAzn.cpp \
    Tracks/Klaster.cpp \
    Tracks/TracksGroup.cpp \
    LMinMax.cpp \
    TrackTrue.cpp \
    Filtrs/BaseFiltr.cpp \
    Filtrs/LineFiltr.cpp \
    Tracks/TrackAzn.cpp \
    AutoWriteJsonFile.cpp \
    Tracks/TrackSource3D.cpp \
    Parsers/ParserExternRls.cpp \
    Tracks/DataForBinding.cpp \
    WriteVideo/VideoSaver.cpp \
    WriteVideo/VideoSaverController.cpp

HEADERS += \
    FMObject.h \
    Settings.h \
    RlkServer.h \
    CreateModuleObj.h \
    ReadObj.h \
    NetObj.h \
    ThreadController.h \
    VoiObj.h \
    TypeVersionForCreate.h \
    Fft_calc.h \
    Fuzzificator.h \
    FiToH.h \
    Parsers/ParserBla.h \
    Parsers/ParserKoir.h \
    Parsers/ParserMkc.h \
    Parsers/ParserPoi.h \
    Parsers/ParserReb.h \
    Parsers/ParserRequest.h \
    Parsers/ParserRmo.h \
    Parsers/ParserRtr.h \
    Parsers/ParserServerBase.h \
    Parsers/ParserSpp.h \
    Parsers/ParserVoiKt.h \
    Modules/ModuleServer.h \
    Modules/ModuleServerBase.h \
    Modules/ModuleServerKoir.h \
    Modules/ModuleServerMkc.h \
    Modules/ModuleServerBla.h \
    Modules/ModuleServerPoi.h \
    Modules/ModuleServerMkcFuzz.h \
    Tracks/TrackBuf.h \
    Tracks/TrackSourceBase.h \
    Tracks/TrackKoir.h \
    Tracks/TrackBla.h \
    AzTimePoi.h \
    Filtrs/SingleLineFiltr.h \
    Tracks/TrackRls.h \
    Tracks/TrackUnion.h \
    Modules/ModuleServerRmo.h \
    Parsers/ParserAsterixBase.h \
    Parsers/ParserAsterixAzn.h \
    AsterixTypes/AznEnums.h \
    AsterixTypes/AsterixConst.h \
    AsterixCategory/AsterixCategory021.h \
    AsterixCategory/AsterixCategory023.h \
    AsterixCategory/AsterixCategoryItem.h \
    AsterixTypes/AsterixAzn023.h \
    AsterixTypes/AsterixAzn021.h \
    AsterixTypes/ReceiveFlagsAzn.h \
    AsterixTypes/AsterixVer247.h \
    AsterixCategory/AsterixCategory247.h \
    ServerSockets/SocketTcpServer.h \
    ServerSockets/SocketUdpAsterix.h \
    SimulatedModules/ModuleSimulatedBase.h \
    Parsers/ParserClientAzn.h \
    SimulatedModules/ModuleSimulatedAzn.h \
    AsterixTypes/AsterixSm034.h \
    AsterixCategory/AsterixCategory034.h \
    Parsers/ParserAsterixSva.h \
    AsterixTypes/AsterixState253.h \
    AsterixTypes/CommandsSva.h \
    AsterixCategory/AsterixCategory253.h \
    AsterixTypes/AsterixState253Header.h \
    SimulatedModules/ModuleSimulatedSva.h \
    AsterixTypes/AsterixTrack048.h \
    AsterixTypes/AsterixTrack048Header.h \
    AsterixCategory/AsterixCategory048.h \
    SimulatedModules/ModuleSimulatedAsterix.h \
    Parsers/ParserClientSva.h \
    Parsers/ParserSva.h \
    Modules/ModuleServerSva.h \
    SimulatedSvaWorkObject.h \
    SimulatedAznWorkObject.h \
    Parsers/ParserAzn.h \
    Modules/ModuleServerAzn.h \
    Tracks/Klaster.h \
    Tracks/TracksGroup.h \
    LMinMax.h \
    TrackTrue.h \
    Filtrs/BaseFiltr.h \
    Filtrs/LineFiltr.h \
    Tracks/TrackAzn.h \
    AutoWriteJsonFile.h \
    Tracks/TrackSource3D.h \
    Parsers/ParserExternRls.h \
    Tracks/DataForBinding.h \
    WriteVideo/VideoSaver.h \
    WriteVideo/VideoSaverController.h

