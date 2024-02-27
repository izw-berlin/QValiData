#include "filteredtracker.h"
#include <QDebug>
using namespace std;
using namespace cv;

/**
 * @brief FilteredTracker::FilteredTracker The constructor for the filtered tracker.
 * @param kalmanFilter A pointer to an OpenCV Kalman Filter instance that will be used to set up the filtered tracker.
 */
FilteredTracker::FilteredTracker(KalmanFilter *kalmanFilter){
    kf = kalmanFilter;
    kalmanPredictX = 0;
    kalmanPredictY = 0;
}

/**
 * @brief FilteredTracker::initTrack Initializes the algorithm state, or if already initialized, resets, the algorithm state.
 * @param frame The image on which to track
 * @param box The bounding box of the shape to track
 */
void FilteredTracker::initTrack(Mat *frame, Rect *box){
    track = TrackerCSRT::create();
    track->init(*frame, *box);
    int centerX = int(box->x + (0.5*box->width));
    int centerY = int(box->y + (0.5*box->height));
    kalmanPredictX = centerX;
    kalmanPredictY = centerY;
    kf->statePost = (Mat_<float>(4, 1) << centerX, centerY, 0, 0);
}

/**
 * @brief FilteredTracker::update Runs one step of the filtered tracker algorithm. Updates the Kalman Filter and the tracker, and
 * tries to correct deviations where possible.
 * @param frame The image on which to track.
 * @param boxes A list of bounding boxes of candidate shapes to consider for re-tracking, if the tracker loses track.
 * @param annotation Points to a Mat on which to draw annotations, for debugging purposes. Otherwise, give it a null pointer.
 * @return true if the algorithm successfully updates on its own. Otherwise, false if it needs manual assistance in re-tracking.
 */
bool FilteredTracker::update(Mat *frame, QList<Rect> *boxes, Mat *annotation){
    Mat kalmanPredict = kf->predict();

    // Get the Kalman predicted values (our expected values)
    kalmanPredictX = int(kalmanPredict.at<float>(0, 0));
    kalmanPredictY = int(kalmanPredict.at<float>(1, 0));

    // Update tracker and get bounding box for actual position
    Rect_<int> trackBox;
    bool trackSuccess = track->update(*frame, trackBox);

    // Sometimes the tracker fails and that wouldn't be so useful, so we default to a null value
    double distKalman2Track = -1;
    if(trackSuccess){
        double trackCenterX = (trackBox.x + 0.5*trackBox.width);
        double trackCenterY = (trackBox.y + 0.5*trackBox.height);

        distKalman2Track = sqrt(pow(trackCenterX-kalmanPredictX, 2) +
                                pow(trackCenterY-kalmanPredictY, 2));
        
        // If annotations are enabled, mark the expected and actual positions, along with a line representing the difference between the two
        if (annotation != nullptr){
            // Draw a box around the tracker-reported object
            rectangle(*annotation, Point(int(trackBox.x), int(trackBox.y)),
                        Point(int(trackBox.x + trackBox.width), int(trackBox.y + trackBox.height)),
                        Scalar(255, 0, 255), 2);
            // Draw a line from the center of tracked object to the point the Kalman filter thinks it should be.
            // The line is green if the Kalman filter's and tracker's positions agree within the margin specified by TRACKER_MAX_DIST.
            // The line turns red if the two disagree.
            line(*annotation, Point(int(trackCenterX), int(trackCenterY)),
                                Point(kalmanPredictX, kalmanPredictY),
                                ((distKalman2Track < TRACKER_MAX_DIST)?Scalar(0, 200, 0):Scalar(0, 0, 255)),
                                2);
        }
        // If the tracker successfully reports an object, we update the Kalman filter with this new information.
        Mat measurement = (Mat_<float>(2, 1) << trackCenterX, trackCenterY);
        kf->correct(measurement);
    }
    
    // Draw a circle around the Kalman-predicted point
    if(annotation != nullptr){
        circle(*annotation, Point(kalmanPredictX, kalmanPredictY), 3, Scalar(0, 128, 255), -1);
    }

    // If we didn't get a valid track, or our Kalman position disagrees with the tracked position, we attempt a re-track on the nearest object.
    if(!trackSuccess || distKalman2Track > TRACKER_MAX_DIST){
        Rect nearest;
        double distNearestBox = nearestBox(kalmanPredictX, kalmanPredictY, boxes, nearest);
        qDebug() << "Lost track, attempting re-track";
        // Try to re-train tracker on the nearest rectangle if it's close enough.
        if(distNearestBox >= 0 && distNearestBox < TRACKER_MAX_DIST){
            qDebug() << "Re-tracked";
            // track->clear();
            track->init(*frame, nearest);
            double boxCenterX = nearest.x + (0.5 * nearest.width);
            double boxCenterY = nearest.y + (0.5 * nearest.height);
            Mat measurement = (Mat_<float>(2, 1) << boxCenterX, boxCenterY);
            kf->correct(measurement);
        }
        // Otherwise, give up and let the user manually re-track.
        else{
            qDebug() << "Kalman filter too far from nearest shape";
            return false;
        }
    }
    return true;
}

/**
 * @brief FilteredTracker::getShapes Edge-detection algorithm that smooths and thresholds, then recognizes contours in an
 * image and returns their bounding boxes as well as the thresholded image. Only returns rectangles with area at least
 * as big as BGS_MIN_AREA, but no bigger than BGS_MAX_AREA.
 * @param imgIn The input image
 * @param blurRadius How much to blur the image (Gaussian blur radius in pixels). Helps to remove "fireflies" or
 * bright spots in the image after it's been thresholded.
 * @param rectsOut A QList of rectangles you provide, in which bounding boxes from the contours will be added.
 * Will re-initialize the list automatically when run.
 * @param imgOut A Mat that will hold the output image. If not needed, pass a nullptr.
 */
void FilteredTracker::getShapes(Mat *imgIn, int blurRadius, QList<Rect> &rectsOut, Mat &imgOut){
    // De-noises the image a bit
    // (https://www.pyimagesearch.com/2016/02/01/opencv-center-of-contour/)
    Mat blur;
    Mat thresh;
    GaussianBlur(*imgIn, blur, Size(blurRadius, blurRadius), 0);
    threshold(blur, thresh, 40, 255, CV_THRESH_BINARY);

    vector<vector<Point> > contours;
    findContours(thresh, contours, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

    rectsOut.clear();
    for(vector<Point> c: contours){
        Rect r = boundingRect(c);
        if (r.width*r.height >= BGS_MIN_AREA && r.width*r.height <= BGS_MAX_AREA){
            rectsOut.append(r);
        }
    }
    thresh.copyTo(imgOut);
}

/**
 * @brief FilteredTracker::nearestBox Finds the rectangle at least as big as the specified size, closest to a specified point.
 * @param x The x-coordinate of the specified point
 * @param y The y-coordinate of the specified point
 * @param boxes A QList of the rectangles to consider
 * @param boxOut A pointer to which the closest qualifying rectangle will be copied.
 * @param minSize The smallest rectangle (by side length) to be considered
 * @return The distance of the center of the closest rectangle to the specified point, or -1 if no rectangle meeting the minimum size is found.
 */
double FilteredTracker::nearestBox(int x, int y, QList<Rect> *boxes, Rect &boxOut, int minSize){
    double closestDist = -1;
    Rect closestRect;
    for(Rect b: *boxes){
        double centerX = (b.x + (b.width * 0.5));
        double centerY = (b.y + (b.height * 0.5));
        double dist = sqrt(pow(x-centerX, 2) + pow(y-centerY, 2));
        if((b.width >= minSize) && (b.height >= minSize) &&
            (closestDist < 0 || closestDist > dist)){
                closestDist = dist;
                closestRect = b;
            }
    }
    boxOut = Rect(closestRect);
    return closestDist;
}

QPoint FilteredTracker::position(){
    return QPoint(kalmanPredictX, kalmanPredictY);
}
