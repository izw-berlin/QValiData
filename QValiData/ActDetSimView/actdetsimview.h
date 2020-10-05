#ifndef ACTDETSIMVIEW_H
#define ACTDETSIMVIEW_H

#include <QFrame>
#include "adxlsim.h"
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include "timeseries.h"
#include "qcustomplot.h"
#include <QListWidgetItem>
#include "motionpath.h"
#include "accelfilterdetector.h"
#include "qcpplottimeseries.h"
#include "simulatortab.h"
#include "Iir.h"

namespace Ui {
class ActDetSimView;
}

class ActDetSimView : public SimulatorTab
{
    Q_OBJECT

public:
    explicit ActDetSimView(QWidget *parent = nullptr);
    ~ActDetSimView() override;
    void attachCap(cv::VideoCapture *cap) override;
    void attachTimeSeries(TimeSeries *ts) override;
    void attachPath(QList<MotionPath *> *paths) override;
    void init() override;

protected:
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void on_buttonApply_clicked();

    void on_vidWidget_positionChanged(int position);

    void on_trackBar_sliderMoved(int position);

    void on_trackBar_sliderPressed();

    void on_trackBar_sliderReleased();

    void on_playButton_clicked();

    void on_forwardButton_clicked();

    void on_backButton_clicked();

    void on_vidWidget_playStateChanged(bool state);

    void updatePathList(QList<MotionPath *> *paths);

    void xAxisChanged(QCPRange range);

    void horzScrollBarChanged(int value);

    void on_zoomBar_sliderMoved(int position);

    void on_clipList_itemClicked(QListWidgetItem *item);

    void on_customPlot_mousePress(QMouseEvent *e);

    void on_customPlot_mouseMove(QMouseEvent *e);

    void on_customPlot_doubleClick(QMouseEvent *e);

    void on_magnify_toggled(bool checked);

    void plotMotionTracks();

    void ADXL_spinbox_valueChanged(QString arg1);

    void on_button_exportcoverage_clicked();

public slots:
    void syncCap() override;
    void syncPath() override;
    void updateSync(double startTime, double rate) override;
    void updateStat(double start, double end) override;
    void clearGraph();

private:
    Ui::ActDetSimView *ui;
    TimeSeries *data;
    AccelFilterDetector *accelSim;

    qreal dataLength;

    QMap<QListWidgetItem *, MotionPath *> * namesToPath;
    QList<MotionPath *> *paths;

    QCPItemStraightLine *plotTrackBar;
    QCPItemStraightLine *plotVidStart;
    QCPItemStraightLine *plotVidEnd;
    QCPItemText *plotVidStartText;
    QCPItemText *plotVidEndText;

    QCPItemStraightLine *plotStatStart;
    QCPItemStraightLine *plotStatEnd;
    QCPItemText *plotStatStartText;
    QCPItemText *plotStatEndText;

    QHash<MotionPath*, double> *pathCoverage;

    qreal deltaTVD;
    qreal rateMultiplier;

    bool hasInit;

    double simStart;
    double simEnd;

    int dataTimeToFrame(double dataTime);
    double frameToDataTime(int frame);

signals:
    void statChanged(double start, double end);
};

#endif // ACTDETSIMVIEW_H
