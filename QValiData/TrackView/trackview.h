#ifndef TRACKVIEW_H
#define TRACKVIEW_H

#include <QFrame>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include "filteredtracker.h"
#include "motionpath.h"
#include <QListWidgetItem>
#include <qcustomplot.h>
#include <timeseries.h>
#include "qcpplottimeseries.h"

using namespace cv;
namespace Ui {
class TrackView;
}

class TrackView : public QFrame
{
    Q_OBJECT

public:
    explicit TrackView(QWidget *parent = nullptr);
    ~TrackView() override;
    void attachCap(VideoCapture *cap);
    void attachTimeSeries(TimeSeries *ts);
    void attachPath(QList<MotionPath *> *paths);
    void init();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void on_player_positionChanged(int position);

    void on_trackBar_sliderMoved(int position);

    void on_trackBar_sliderPressed();

    void on_trackBar_sliderReleased();

    void on_playButton_clicked();

    void on_forwardButton_clicked();

    void on_backButton_clicked();

    void on_playState_changed(bool state);

    void updatePathList();

    void selectPath(MotionPath *path);

    void on_clipList_itemClicked(QListWidgetItem *item);

    void on_trimButton_clicked();

    void plotMotionTracks();

    void xAxisChanged(QCPRange range);

    void horzScrollBarChanged(int value);

    void on_zoomBar_sliderMoved(int position);

    void on_customPlot_mousePress(QMouseEvent *e);

    void on_customPlot_mouseMove(QMouseEvent *e);

    void on_removeButton_clicked();

    void on_button_addAnnot_clicked();

    void on_button_addAnnot_toggled(bool checked);

    void on_checkBox_showPaths_toggled(bool checked);

public slots:
    void syncCap();
    void syncPath();
    void clearGraph();
    void updateSync(double startTime, double rate);
    void on_customPlot_doubleClick(QMouseEvent * e);

private:
    Ui::TrackView *ui;

    TimeSeries *data;

    QCPItemStraightLine *plotTrackBar;
    QCPItemStraightLine *plotVidStart;
    QCPItemStraightLine *plotVidEnd;
    QCPItemText *plotVidStartText;
    QCPItemText *plotVidEndText;
    qreal deltaTVD;
    qreal rateMultiplier;

    //Should the player continue playing after the trackbar was released?
    bool resumeOnRelease;

    QList<MotionPath *> *paths;
    QMap<QListWidgetItem *, MotionPath *> * namesToPath;

    qreal dataLength;
    bool hasInit;
    bool addingManualAnnotation;
    int manualAdd1;
    int manualAdd2;
};

#endif // TRACKVIEW_H
