#DEPENDPATH += ../Voi_Common
#INCLUDEPATH += ../Voi_Common

include(../Voi_Common/Voi_Common.pri)
include(../Common/Common.pri)
include(../Common/UI/CommonUI.pri)
include(AskuComplex/AskuComplex.pri)
include(asku-gui/asku-gui.pri)

QT += gui
QT += widgets


TEMPLATE = app

DESTDIR = ../bin


CONFIG(debug, debug|release) {
  TARGET = Voi_Client_d
}
else {
  TARGET = Voi_Client
}

SOURCES += main.cpp\
    CopyDialog.cpp \
    FileManager.cpp \
    HalfDlg.cpp \
    LogWindow.cpp \
    NetThread.cpp \
    Plots/PlotBaseClient.cpp \
    Plots/PlotRlsClient.cpp \
    RecreateDlg.cpp \
    Settings_Cl.cpp \
    BaseIKO.cpp \
    CtrlWnd.cpp \
    MainIko.cpp \
    MasshIko.cpp \
    ServersDlg.cpp \
    UserDlg.cpp \
    ModulWnd.cpp \
    RlkClient.cpp \
    RlkWnd.cpp \
    PositionWnd.cpp \
    BaseInfoWnd.cpp \
    ParserVoiServer.cpp \
    FormPlace.cpp \
    Coord.cpp \
    ScaleOpt.cpp \
    FormBase.cpp \
    Form.cpp \
    KT_WRL.cpp \
    KT_AZN.cpp \
    KT_Bla.cpp \
    ThreadController.cpp \
    Figures/FigInfoSpp.cpp \
    Figures/FigKT_Async.cpp \
    Figures/FigKT_AZN.cpp \
    Figures/FigKT_Bla.cpp \
    Figures/FigKT_Cross.cpp \
    Figures/FigKT_WRL.cpp \
    Figures/FigMoveLine.cpp \
    Figures/FigRubberBand.cpp \
    Figures/FigScale.cpp \
    Figures/FigTrack.cpp \
    Figures/FigPlotRls.cpp \
    Figures/PlotRlsDrawInterface.cpp \
    Figures/FigRezhSector.cpp \
    Figures/FigZonaObz.cpp \
    Figures/FigZonaObzPoi.cpp \
    Modules/ModuleClientBase.cpp \
    RlkContainer.cpp \
    Modules/ModuleClientKoir.cpp \
    Modules/ModuleClientPoi.cpp \
    Modules/ModuleClientMkc.cpp \
    Modules/ModuleClientBla.cpp \
    Modules/ModuleClientSva.cpp \
    Modules/ModuleClientAzn.cpp \
    Modules/ModuleClientRmo.cpp \
    Windows/InfoWnd.cpp \
    Figures/FigBase.cpp \
    Figures/FigBaseSlow.cpp \
    Figures/FigSled.cpp \
    TrackSledClient.cpp \
    InfoItemBuf.cpp

HEADERS  += \
    CopyDialog.h \
    FileManager.h \
    HalfDlg.h \
    LogWindow.h \
    NetThread.h \
    Plots/PlotBaseClient.h \
    Plots/PlotRlsClient.h \
    RecreateDlg.h \
    Settings_Cl.h \
    BaseIKO.h \
    CtrlWnd.h \
    MainIko.h \
    MasshIko.h \
    ServersDlg.h \
    ClientConstant.h \
    UserDlg.h \
    ModulWnd.h \
    RlkClient.h \
    RlkWnd.h \
    PositionWnd.h \
    BaseInfoWnd.h \
    ParserVoiServer.h \
    FormPlace.h\
    Coord.h \
    BmpPlot.h \
    ScaleOpt.h \
    FormBase.h \
    Form.h \
    ConstRLI.h \
    KT_WRL.h \
    KT_AZN.h \
    KT_Bla.h \
    KT_Cross.h \
    ThreadController.h \
    Figures/FigInfoSpp.h \
    Figures/FigInfoSpp_copy.h \
    Figures/FigKT_Async.h \
    Figures/FigKT_AZN.h \
    Figures/FigKT_Bla.h \
    Figures/FigKT_Cross.h \
    Figures/FigKT_WRL.h \
    Figures/FigMoveLine.h \
    Figures/FigRubberBand.h \
    Figures/FigScale.h \
    Figures/FigTrack.h \
    Figures/FigPlotRls.h \
    Figures/PlotRlsDrawInterface.h \
    DrawConst/ConstColors.h \
    DrawConst/EnumPlotTypes.h \
    DrawConst/EnumColorInd.h \
    Figures/FigRezhSector.h \
    Figures/FigZonaObz.h \
    Figures/FigZonaObzPoi.h \
    Modules/ModuleClientBase.h \
    RlkContainer.h \
    Modules/ModuleClientPoi.h \
    Modules/ModuleClientKoir.h \
    Modules/ModuleClientMkc.h \
    Modules/ModuleClientBla.h \
    Modules/ModuleClientSva.h \
    Modules/ModuleClientAzn.h \
    Modules/ModuleClientRmo.h \
    Windows/InfoWnd.h \
    Figures/FigBase.h \
    Figures/FigBaseSlow.h \
    Figures/FigSled.h \
    TrackSledClient.h \
    InfoItemBuf.h
