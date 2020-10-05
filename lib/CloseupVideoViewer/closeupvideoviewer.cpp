#include "closeupvideoviewer.h"

using namespace cv;

CloseupVideoViewer::CloseupVideoViewer(QWidget *parent) : OpenCVVideoPlayer (parent)
{
    paths = new QList<MotionPath *>();
    currentPath = nullptr;
    thePoint = QPoint(0, 0);
}

void CloseupVideoViewer::attachPath(QList<MotionPath *> *paths){
    this->paths = paths;
}

void CloseupVideoViewer::frameUpdate(){
    frameNumber = int(cap->get(CAP_PROP_POS_FRAMES));
    for(int i=0; i<paths->size(); ++i){
        MotionPath *m = paths->at(i);
        if(frameNumber >= (m->start - 1) && frameNumber <= (m->end + 1)){
            currentPath = m;
            break;
        }
    }
    if(playing){
        QTime timercomp;
        timercomp.start();
        bool result = advanceFrame();
        if(!result){
            emit playStateChanged(false);
            return;
        }
        else{
            emit positionChanged(frameNumber);
            // Sets next frame timer, compensating for processing overhead
            frameTimer->start(qMax(1, int(frameInterval - timercomp.elapsed())));
        }
    }
    else{ //Stop when we can't get any more frames
        emit playStateChanged(false);
    }
    if(!frameIn->empty()){
        imshow(*frameIn);
    }
}

void CloseupVideoViewer::imshow(Mat &img){
    if(showMagnifier){
        Mat img_clone = img.clone();
        if(currentPath != nullptr && currentPath->contains(frameNumber)){
            thePoint = currentPath->getPoint(frameNumber);
        }

        int frame_w = img_clone.cols;
        int frame_h = img_clone.rows;

        int pip_w = int(frame_w * INSETFRACTION);
        int pip_h = int(frame_h * INSETFRACTION);

        int roi_w = int(pip_w/ZOOMFACTOR);
        int roi_h = int(pip_h/ZOOMFACTOR);

        int roi_top = int(qMax(0, qMin(frame_h - roi_h, thePoint.y() - roi_h/2)));
        int roi_left = int(qMax(0, qMin(frame_w - roi_w, thePoint.x() - roi_w/2)));

        Mat roi = img_clone(Rect(roi_left, roi_top, roi_w, roi_h)).clone();
        if(!roi.empty()){
            Mat pip;
            cv::resize(roi, pip, (Size(pip_w, pip_h)));

            int pip_top = int(qMax(0, qMin(frame_h-pip_h, thePoint.y()-(pip_h/2))));
            int pip_left = int(qMax(0, qMin(frame_w-pip_w, thePoint.x()-(pip_w/2))));

            pip.copyTo(img_clone(Rect(pip_left, pip_top, pip_w, pip_h)));
        }
        OpenCVDisplay::imshow(img_clone);
    }
    else{
        OpenCVDisplay::imshow(img);
    }
}

void CloseupVideoViewer::setMagnify(bool magnify){
    showMagnifier = magnify;
}
