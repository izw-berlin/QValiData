#include "bgsfilteredtracker.h"
#include <QDebug>

using namespace cv;

BGSFilteredTracker::BGSFilteredTracker(QWidget *parent) : OpenCVVideoPlayer(parent)
{
    connect(frameTimer, SIGNAL(timeout()), this, SLOT(frameUpdate()));
    connect(this, SIGNAL(mouseMovedRelative(QMouseEvent *)), this, SLOT(onMouseMove(QMouseEvent *)));
    connect(this, SIGNAL(mousePressedRelative(QMouseEvent *)), this, SLOT(onMousePress(QMouseEvent *)));
}

/**
 * @brief BGSFilteredTracker::attachtoPathList Attaches this video tracker to an external path list.
 * @param paths A pointer to an external path list
 */
void BGSFilteredTracker::attachtoPathList(QList<MotionPath *> *paths){
    this->paths = paths;
}
void BGSFilteredTracker::init(){
    fgbg = createBackgroundSubtractorMOG2(20, 16, false);
    frameIn = new Mat();
    frameBGS = new Mat();
    frameOut = new Mat();

    showPaths = true;

    kalmanFilter = new KalmanFilter(4, 2, 0, CV_32F);
    float dt = 10.;
    kalmanFilter->transitionMatrix = (Mat_<float>(4, 4) <<  1, 0, dt, 0,
                                                            0, 1, 0, dt,
                                                            0, 0, 1, 0,
                                                            0, 0, 0, 1);

    kalmanFilter->measurementMatrix = (Mat_<float>(2, 4) << 1, 0, 0, 0,
                                                            0, 1, 0, 0);

    double processNoiseCoeff = 1e-4;
    // Gets us a 4x4 identity matrix times processNoiseCoeff.
    setIdentity(kalmanFilter->processNoiseCov, Scalar::all(processNoiseCoeff));

    double measurementNoiseCoeff = 0.05;
    // Gets us a 2x2 identity matrix times measurementNoiseCoeff
    setIdentity(kalmanFilter->measurementNoiseCov, Scalar::all(measurementNoiseCoeff));

    kalmanFilter->errorCovPost = Scalar(1);
    // Keep scrolling until we get a non-empty frame, or we reach a limit.

    while(frameIn->empty() && frameNumber < MAX_PREREAD_FRAMES){
        cap->read(*frameIn);
        frameNumber ++;
    }

    // Frames are initialized to black, so we need to grab an actual image frame to
    // initialize our background subtractor, otherwise it'll flash really bright for a frame.
    Mat temp;
    fgbg->apply(*frameIn, temp);
    state = START;
    imshow(*frameIn);

    selectedRect = nullptr;
    hoveredRect = nullptr;

    comboTrack = new FilteredTracker(kalmanFilter);
    rects = new QList<Rect>();

    currentPath = nullptr;
    if(paths == nullptr)
        paths = new QList<MotionPath *>();

    // Make the playhead go back to its original position after pre-reading a frame.
    seek(0);
}
/**
 * @brief BGSFilteredTracker::drawRectOutline Draws an OpenCV rectangle, but bigger by a specified width.
 * @param img The output image on which to draw the rectangle
 * @param rect The original OpenCV rectangle
 * @param expandWidth How much bigger on each side to make this rectangle, in pixels
 * @param color The color of the rectangle
 * @param width The outline width of the rectangle
 */
void BGSFilteredTracker::drawRectOutline(InputOutputArray img, Rect rect, int expandWidth, const Scalar &color, int width){
    rectangle(img, Point(rect.x - expandWidth, rect.y - expandWidth),
              Point(rect.x + rect.width + expandWidth, rect.y + rect.height + expandWidth),
              color, width);
}

void BGSFilteredTracker::printState(){
    switch(state){
    case NOMOTION:
        qDebug() << "State: NOMOTION";
        break;

    case WAITSELECT:
        qDebug() << "State: WAITSELECT";
        break;

    case TRACK:
        qDebug() << "State: TRACK";
        break;

    case SEEK:
        qDebug() << "State: SEEK";
        break;

    case START:
        qDebug() << "State: START";
        break;

    case PLAYING:
        qDebug() << "State: PLAYING";
        break;
    }
}
void BGSFilteredTracker::frameUpdate(){
    currentPath = nullptr;
    frameNumber = int(cap->get(CAP_PROP_POS_FRAMES));
    for(int i=0; i<paths->size(); ++i){
        MotionPath *m = paths->at(i);
        if(frameNumber >= (m->start - 1) && frameNumber <= (m->end + 1)){
            currentPath = m;
            break;
        }
    }
    emit pathSelected(currentPath);
    if(playing || state == SEEK || state == WAITSELECT){
        QTime timercomp;
        timercomp.start();

        if(state != WAITSELECT && state != SEEK){
            bool result = advanceFrame();
            if(!result){
                return;
            }
            else{
                emit positionChanged(frameNumber);
            }
        }
        //addWeighted(*frameBGS, 0.5, *frameIn, 0.5, 0, *frameOut);
        *frameOut = frameIn->clone();
        // If we don't detect any motion, we tell the tracker to skip ahead until we find motion again.
        if(rects->empty() && state != SEEK && state != TRACK && state != PLAYING){
            state = NOMOTION;
        }
        // Once we find motion again, wait until the user selects a shape.
        else if(state == NOMOTION){
            state = WAITSELECT;
            playing = false;
            emit playStateChanged(false);
        }

        if(state == TRACK){
            bool track_success = comboTrack->update(frameBGS, rects, frameOut);
            if(!track_success){ // If we lose track, wait for manual re-tracking                          
                playing = false;
                emit playStateChanged(false);
                state = WAITSELECT;
            }
            else {
                addPoint(frameNumber, comboTrack->position());
                frameTimer->start(qMax(1, int(frameInterval - timercomp.elapsed())));
            }
        }

        // If we're just playing already-tracked footage, just play as normal.
        if(state == PLAYING){
            if (currentPath != nullptr && !currentPath->contains(frameNumber)){
                state = NOMOTION;
            }
            else{
                frameTimer->start(qMax(1, int(frameInterval - timercomp.elapsed())));
            }
        }

        if(state == NOMOTION){
            frameTimer->start(1); // Some fast frame timer to skip over boring frames;
            if (currentPath != nullptr && currentPath->contains(frameNumber)){
                state = PLAYING;
            }
        }

        if (state == WAITSELECT || state == SEEK){
            if((state == WAITSELECT && (currentPath == nullptr ||
                                        (currentPath != nullptr && !currentPath->contains(frameNumber)))) ||
                    state == SEEK){
                for (Rect r: *rects){
                    drawRectOutline(*frameOut, r, 10, Scalar(0, 64, 0), 2);
                }
                if(hoveredRect != nullptr){
                    drawRectOutline(*frameOut, *hoveredRect, 10, Scalar(0, 200, 0), 2);
                }
                if(selectedRect != nullptr){
                    drawRectOutline(*frameOut, *selectedRect, 10, Scalar(0, 255, 0), 2);
                }
            }
            else if(state == WAITSELECT && currentPath->contains(frameNumber)){    
                state = PLAYING;
            }
        }
        if(state == START){
            state = NOMOTION;
            frameTimer->start(1); // Some fast timer to get another frame
        }
        // Draw path
        if(currentPath != nullptr){
            if(currentPath->contains(frameNumber)){
                QPoint thePoint = currentPath->getPoint(frameNumber);
                // Draw shapes twice--first in black, and then thinner in white for enhanced contrast
                circle(*frameOut, Point(thePoint.x(), thePoint.y()), 20, Scalar(0, 0, 0, 255), THICKNESS_LINE_OUTER);
                circle(*frameOut, Point(thePoint.x(), thePoint.y()), 20, Scalar(255, 255, 255, 255), THICKNESS_LINE_INNER);

                if(showPaths){
                    // Two loops for drawing paths because line endcaps cause strange visual effects when drawn simultaneously
                    QPoint lastPoint = currentPath->getPoint(currentPath->start);
                    for(int i=1; i<(currentPath->end - currentPath->start); i++){
                        QPoint nextPoint = currentPath->getPoint(currentPath->start + i);
                        line(*frameOut, Point(nextPoint.x(), nextPoint.y()),
                             Point(lastPoint.x(), lastPoint.y()),
                             Scalar(0, 0, 0), THICKNESS_LINE_OUTER);
                        lastPoint = QPoint(nextPoint);
                    }
                    lastPoint = currentPath->getPoint(currentPath->start);
                    for(int i=1; i<(currentPath->end - currentPath->start); i++){
                        QPoint nextPoint = currentPath->getPoint(currentPath->start + i);
                        line(*frameOut, Point(nextPoint.x(), nextPoint.y()),
                             Point(lastPoint.x(), lastPoint.y()),
                             Scalar(255, 255, 255), THICKNESS_LINE_INNER);
                        lastPoint = QPoint(nextPoint);
                    }
                }
            }
        }
        if(!frameOut->empty())
            imshow(*frameOut);

    }
    else{ //Stop when reached end of playback
        if(!frameIn->empty()) //Show a "preview" of current frame, if available
            imshow(*frameIn);
        playing = false;
        emit playStateChanged(false);
    }
}

void BGSFilteredTracker::trainTracker(Mat *frame, Rect *rect){
    comboTrack->initTrack(frame, rect);
}

void BGSFilteredTracker::onMousePress(QMouseEvent *e){
    Q_UNUSED(e)
    if(state == WAITSELECT || state == SEEK){
        if(hoveredRect != nullptr){
            selectedRect = new Rect(*hoveredRect);
            Rect *newRect = new Rect(selectedRect->x - WIDTH_RETRACK_EXPAND,
                                     selectedRect->y - WIDTH_RETRACK_EXPAND,
                                     selectedRect->width + (2*WIDTH_RETRACK_EXPAND),
                                     selectedRect->height + (2*WIDTH_RETRACK_EXPAND));
            trainTracker(frameBGS, newRect);
            addPoint(frameNumber, comboTrack->position());
            selectedRect = nullptr;
            hoveredRect = nullptr;
            state = TRACK;
            playing = true;
            emit playStateChanged(true);
        }
    }
    frameUpdate();
}

void BGSFilteredTracker::onMouseMove(QMouseEvent *e){
    if(state == WAITSELECT || state == SEEK){
        hoveredRect = nullptr;
        Rect closest;
        double dist = FilteredTracker::nearestBox(e->x(), e->y(), rects, closest);
        if (dist >= 0){
            if((e->x() > (closest.x - 10) && e->x() < (closest.x + closest.width + 10)) &&
                    (e->y() > (closest.y - 10) && e->y() < (closest.y + closest.height + 10))){
                hoveredRect = new Rect(closest);
            }
        }
        frameUpdate();
    }

}

bool BGSFilteredTracker::advanceFrame(){
    bool result = cap->read(*frameIn);
    if(!result){
        playing = false;
        emit playStateChanged(playing);
        qWarning() << "Had trouble getting frame";
        return false;
    }
    // OpenCV automatically advances frame number when you read a frame, hence the minus one.
    Mat fgmask;
    fgbg->apply(*frameIn, fgmask);
    Mat frameBGS_mono;
    FilteredTracker::getShapes(&fgmask, BGS_BLUR_RADIUS, *rects, frameBGS_mono);
    cvtColor(frameBGS_mono, *frameBGS, COLOR_GRAY2BGR);
    return true;
}

bool BGSFilteredTracker::reverseFrame(){
    if(frameNumber > 0){
        state = SEEK;
        cap->set(CAP_PROP_POS_FRAMES, cap->get(CAP_PROP_POS_FRAMES) - 2);
        bool result = advanceFrame();
        if(result){
            emit positionChanged(frameNumber);
            return true;
        }
    }
    return false;
}

void BGSFilteredTracker::keyPressEvent(QKeyEvent *e){
    switch(e->key()){

    case Qt::Key_Left:
        reverseFrame();
        frameUpdate();
        e->accept();
        break;

    case Qt::Key_Right:
        advanceFrame();
        frameUpdate();
        e->accept();
        break;
    }
}

void BGSFilteredTracker::seek(int framenumber_new){
    // Constrains frame number within first and last frame numbers.
    int targetFrame = qMin(qMax(0, framenumber_new), videoLength-1);
    int oldFrame = frameNumber;

    // Only emit the change signal if frame number actually changes.
    if(targetFrame != oldFrame){
        cap->set(CAP_PROP_POS_FRAMES, targetFrame);
        //Update the playback window with a "preview" of the current frame
        state = SEEK;
        advanceFrame();
        frameUpdate();
        // Move playhead back to where it was prior to grabbing a preview frame
        cap->set(CAP_PROP_POS_FRAMES, targetFrame);

        frameNumber = targetFrame;
        emit positionChanged(frameNumber);
    }
}

void BGSFilteredTracker::play(){
    if(!playing){
        playing = true;
        emit playStateChanged(true);
    }
    state = START;
    frameUpdate();
}

void BGSFilteredTracker::pause(){
    if(playing){
        playing = false; //Will just wait until the last timer times out before continuing.
        emit playStateChanged(false);
    }
    state = SEEK;
    frameUpdate();
}

bool BGSFilteredTracker::isPlaying(){
    return playing;
}

void BGSFilteredTracker::addPoint(int frame, QPoint point){
    if(currentPath == nullptr){
        int insertAt = 0;
        // Insert but maintain order of paths
        for(insertAt = 0; insertAt < paths->length(); ++insertAt){
            if(paths->at(insertAt)->start > frame)
                break;
        }
        paths->insert(insertAt, new MotionPath());
        currentPath = paths->at(insertAt);
    }
    currentPath->putPoint(frame, point);

    // Try to merge adjacent paths by merging "forwards"
    int currentIndex = 0;

    // Loop until we reach end of list (or equivalently are left with only one item)
    while(currentIndex < paths->length()-1){
        // Join if paths are adjacent
        MotionPath *current = paths->at(currentIndex);
        MotionPath *next = paths->at(currentIndex + 1);
        if(current->end >= (next->start - 1)){
            // Copy points over to previous path, starting at end of current path
            for(int i=current->end + 1; i<next->end; i++){
                current->putPoint(i, next->getPoint(i));
            }
            // Transfer pointer if we're deleting the current path
            if(currentPath == next)
                currentPath = current;
            paths->removeAt(currentIndex+1);
        }
        else{
            currentIndex ++;
        }
    }
    emit pathsUpdated();
}

void BGSFilteredTracker::seekToPath(MotionPath *path){
    seek(path->start);
}

QList<MotionPath *> *BGSFilteredTracker::getPaths(){
    return paths;
}

void BGSFilteredTracker::setShowPaths(bool show){
    showPaths = show;
}
