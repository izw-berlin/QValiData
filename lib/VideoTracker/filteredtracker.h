#ifndef FILTEREDTRACKER_H
#define FILTEREDTRACKER_H

#include <QObject>
#include <QPoint>
#include <opencv2/video/tracking.hpp>
#include <opencv2/tracking/tracker.hpp>

#define BGS_MIN_AREA 25
#define BGS_MAX_AREA 5000
#define TRACKER_MAX_DIST 30

class FilteredTracker
{
public:
    FilteredTracker(cv::KalmanFilter *kalmanFilter);
    void initTrack(cv::Mat *frame, cv::Rect *box);
    bool update(cv::Mat *frame, QList<cv::Rect> *boxes, cv::Mat *annotation=nullptr);
    static void getShapes(cv::Mat *imgIn, int blurRadius, QList<cv::Rect> &rectsOut, cv::Mat &imgOut);
    static double nearestBox(int x, int y, QList<cv::Rect> *boxes, cv::Rect &boxOut, int minSize=0);
    QPoint position();
private:
    cv::KalmanFilter *kf;
    cv::Ptr<cv::Tracker> track;
    enum {NOMOTION, WAITSELECT, TRACK, REVERSE} state;
    int kalmanPredictX, kalmanPredictY;

};

#endif // FILTEREDTRACKER_H
