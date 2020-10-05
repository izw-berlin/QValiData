#ifndef SYNCVIEW_H
#define SYNCVIEW_H

#include <QFrame>
#include <QFile>
#include <QDir>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include "timeseries.h"
#include "qcustomplot.h"
#include "qcpplottimeseries.h"

namespace Ui {
class SyncView;
}

class SyncView : public QFrame
{
    Q_OBJECT

public:
    explicit SyncView(QFrame *parent = nullptr);
    ~SyncView() override;
    void attachCap(cv::VideoCapture *cap);
    void attachTimeSeries(TimeSeries *ts);
    void init();
    void updateSync(double start, double rate);

signals:
    void syncChanged(double startTime, double rate);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void on_vidWidget_positionChanged(int position);

    void on_trackBar_sliderMoved(int position);

    void on_trackBar_sliderPressed();

    void on_trackBar_sliderReleased();

    void on_playButton_clicked();

    void on_forwardButton_clicked();

    void on_backButton_clicked();

    void on_vidWidget_playStateChanged(bool state);

    void xAxisChanged(QCPRange range);

    void on_customPlot_mouseMove(QMouseEvent *e);

    void horzScrollBarChanged(int value);

    void on_zoomBar_sliderMoved(int position);

    void on_lineEdit_editingFinished();

    void on_button_syncReset_clicked();

    void on_button_rateReset_clicked();

    void on_customPlot_selectionChangedByUser();

    void syncChange();

public slots:
    void syncCap();

private:
    Ui::SyncView *ui;

    TimeSeries *data;

    qreal dataLength; // Length of data in seconds

    qreal deltaTVD; // Number of seconds between the start of data and start of video (i.e. video start time - data start time)
    qreal rateMultiplier; // How many seconds in data equals one second in video

    QCPItemStraightLine *plotTrackBar;
    QCPItemStraightLine *plotVidStart;
    QCPItemStraightLine *plotVidEnd;
    QCPItemText *plotVidStartText;
    QCPItemText *plotVidEndText;
};

#endif // MAINWINDOW_H
