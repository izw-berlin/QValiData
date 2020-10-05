#ifndef BGSFILTEREDTRACKER_H
#define BGSFILTEREDTRACKER_H

#include <QObject>
#include <QWidget>
#include <QTime>
#include "opencvvideoplayer.h"
#include "filteredtracker.h"
#include "motionpath.h"

#define MAX_PREREAD_FRAMES 20
#define WIDTH_RETRACK_EXPAND 10
#define BGS_BLUR_RADIUS 21

class BGSFilteredTracker : public OpenCVVideoPlayer
{
    Q_OBJECT
public:
    explicit BGSFilteredTracker(QWidget *parent = nullptr);
    void init();
    enum {NOMOTION, WAITSELECT, TRACK, SEEK, START, PLAYING} state;

    void seek(int framenumber_new) override;
    void play() override;
    void pause() override;
    void trainTracker(cv::Mat *frame, cv::Rect *rect);
    bool advanceFrame() override;
    bool reverseFrame();

    void drawRectOutline(cv::InputOutputArray img, cv::Rect rect, int expandWidth, const cv::Scalar &color, int width);

    void seekToPath(MotionPath * path);
    QList<MotionPath *> *getPaths();

    void attachtoPathList(QList<MotionPath *> *paths);

    bool isPlaying();
protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    cv::KalmanFilter *kalmanFilter;
    cv::Mat *frameBGS; // Resulting frame from background subtraction
    cv::Mat *frameOut; // Output frame of video, including all overlays
    cv::Ptr<cv::BackgroundSubtractor> fgbg;

    FilteredTracker *comboTrack;
    cv::Rect *selectedRect;
    cv::Rect *hoveredRect;
    QList<cv::Rect> *rects;
    void printState();

    MotionPath *currentPath;
    QList<MotionPath *> *paths;
    void addPoint(int frame, QPoint point);

signals:
    void pathsUpdated();
    void pathSelected(MotionPath * path);

public slots:
    void frameUpdate() override;
    void onMousePress(QMouseEvent *e);
    void onMouseMove(QMouseEvent *e);
};

#endif // BGSFILTEREDTRACKER_H
