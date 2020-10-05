#ifndef SIMULATORTAB_H
#define SIMULATORTAB_H

#include <QObject>
#include <QWidget>
#include <QFrame>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include "timeseries.h"
#include "motionpath.h"

class SimulatorTab : public QFrame
{
    Q_OBJECT
public:
    explicit SimulatorTab(QWidget *parent = nullptr);
    virtual void attachCap(cv::VideoCapture *cap) = 0;
    virtual void attachTimeSeries(TimeSeries *ts) = 0;
    virtual void attachPath(QList<MotionPath *> *paths) = 0;
    virtual void init() = 0;

public slots:
    virtual void syncCap() = 0;
    virtual void syncPath() = 0;
    virtual void updateSync(double startTime, double rate) = 0;
    virtual void updateStat(double start, double end) = 0;

signals:

};

#endif // SIMULATORTAB_H
