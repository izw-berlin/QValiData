#-------------------------------------------------
#
# Project created by QtCreator 2019-07-24T15:52:09
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = QValiData
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

INCLUDEPATH += \
        /usr/local/include/opencv4 \
        /usr/include/iir \
        /usr/local/include \
        ActDetSimView \
        ADXLSimView \
        FileSelector \
        SyncView \
        TrackView \
        ../lib/ActivityDetector \
        ../lib/ADXLSim \
        ../lib/CloseupVideoViewer \
        ../lib/MotionPath \
        ../lib/OpenCVDisplay \
        ../lib/OpenCVVideoPlayer \
        ../lib/QCPPlotTimeSeries \
        ../lib/QCustomPlot \
        ../lib/SimulatorTab \
        ../lib/TimeSeries \
        ../lib/VideoTracker

LIBS += \
        -L/usr/local/lib \
        -L /usr/include \
        -liir \
        -lopencv_core \
        -lopencv_imgproc \
        -lopencv_videoio \
        -lopencv_video \
        -lopencv_tracking

SOURCES += \
        ../lib/ADXLSim/adxlsim.cpp \
        ../lib/ActivityDetector/activitydetector.cpp \
        ../lib/CloseupVideoViewer/closeupvideoviewer.cpp \
        ../lib/MotionPath/motionpath.cpp \
        ../lib/OpenCVVideoPlayer/opencvvideoplayer.cpp \
        ../lib/OpenCVDisplay/opencvdisplay.cpp \
        ../lib/QCPPlotTimeSeries/qcpplottimeseries.cpp \
        ../lib/QCustomPlot/qcustomplot.cpp \
        ../lib/SimulatorTab/simulatortab.cpp \
        ../lib/TimeSeries/timeseries.cpp \
        ../lib/VideoTracker/bgsfilteredtracker.cpp \
        ../lib/VideoTracker/filteredtracker.cpp \
        ADXLSimView/adxlsimview.cpp \
        ActDetSimView/actdetsimview.cpp \
        ActDetSimView/accelfilterdetector.cpp \
        FileSelector/fileselector.cpp \
        SyncView/syncview.cpp \
        TrackView/trackview.cpp \
        main.cpp \
        mainwindow.cpp \

HEADERS += \
        ../lib/ADXLSim/adxlsim.h \
        ../lib/ActivityDetector/activitydetector.h \
        ../lib/CloseupVideoViewer/closeupvideoviewer.h \
        ../lib/MotionPath/motionpath.h \
        ../lib/OpenCVVideoPlayer/opencvvideoplayer.h \
        ../lib/OpenCVDisplay/opencvdisplay.h \
        ../lib/QCPPlotTimeSeries/qcpplottimeseries.h \
        ../lib/QCustomPlot/qcustomplot.h \
        ../lib/SimulatorTab/simulatortab.h \
        ../lib/TimeSeries/timeseries.h \
        ../lib/VideoTracker/bgsfilteredtracker.h \
        ../lib/VideoTracker/filteredtracker.h \
        ActDetSimView/accelfilterdetector.h \
        ActDetSimView/actdetsimview.h \
        ADXLSimView/adxlsimview.h \
        FileSelector/fileselector.h \
        SyncView/syncview.h \
        TrackView/trackview.h \
        mainwindow.h

FORMS += \
        ActDetSimView/actdetsimview.ui \
        ADXLSimView/adxlsimview.ui \
        FileSelector/fileselector.ui \
        SyncView/syncview.ui \
        TrackView/trackview.ui \
        mainwindow.ui

QMAKE_LFLAGS += -no-pie

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
