#ifndef ADXLSIMVIEW_H
#define ADXLSIMVIEW_H

#include <QFrame>
#include "adxlsim.h"
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include "timeseries.h"
#include "qcustomplot.h"
#include <QListWidgetItem>
#include "motionpath.h"
#include "qcpplottimeseries.h"
#include <QMessageBox>
#include "simulatortab.h"

namespace Ui {
class ADXLSimView;
}

class ADXLSimView : public SimulatorTab
{
    Q_OBJECT

public:
    explicit ADXLSimView(QWidget *parent = nullptr);
    ~ADXLSimView() override;
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

    void on_checkBox_adxlWakeup_stateChanged(int arg1);

    void on_button_exportactive_clicked();

public slots:
    void syncCap() override;
    void syncPath() override;
    void updateSync(double startTime, double rate) override;
    void updateStat(double start, double end) override;
    void clearGraph();

private:
    Ui::ADXLSimView *ui;
    TimeSeries *data;
    ADXLSim2 *adxl;

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

    // Counts how many active samples cover each given path, in frames
    QHash<MotionPath*, double> *pathCoverage;

    // double = length of active region, in seconds. true = correct, false = false positive
    QList<QPair<double, bool>> activeRegionList;

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

#endif // ADXLSIM_H
