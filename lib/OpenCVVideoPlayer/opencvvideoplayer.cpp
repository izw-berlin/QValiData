#include "opencvvideoplayer.h"
#include <QTime>
#include <QDebug>

using namespace cv;

OpenCVVideoPlayer::OpenCVVideoPlayer(QWidget *parent) : OpenCVDisplay(parent)
{
    frameTimer = new QTimer();
    frameTimer->setSingleShot(true);
    cap = new VideoCapture();
    frameNumber = 0;
    videoLength = 0;
    playing = false;
    fps = 0;
    frameInterval = 0;

    frameIn = new Mat();
    connect(frameTimer, SIGNAL(timeout()), this, SLOT(frameUpdate()));
}

void OpenCVVideoPlayer::getCapProperties(){
    frameNumber = int(cap->get(CAP_PROP_POS_FRAMES));
    videoLength = int(cap->get(CAP_PROP_FRAME_COUNT));
    fps = cap->get(CAP_PROP_FPS);
    //Set frame rate if nonzero. Else, default to 30fps if zero or close to zero
    if(fabs(fps) >= .001){
        frameInterval = int(1000.0/fps);
    }
    else{
        frameInterval = int(1000.0/30);
    }
}
void OpenCVVideoPlayer::attachCap(VideoCapture *cap){
    this->cap = cap;
    if(cap->isOpened()){
        getCapProperties();
        playing = false;
    }
}

bool OpenCVVideoPlayer::openVideoFile(QString filename){
    // Doesn't try to re-open the capture device if it's already opened somewhere else
    bool result = cap->isOpened() || cap->open(filename.toStdString());
    if(result){
        qDebug() << "Successfully opened video file";
        getCapProperties();
        playing = false;
    }
    return result;
}

void OpenCVVideoPlayer::play(){
    playing = true;
    frameTimer->start(int(frameInterval));
    emit playStateChanged(true);
}

void OpenCVVideoPlayer::pause(){
    playing = false; //Will just wait until the last timer times out before continuing.
    emit playStateChanged(false);
}

void OpenCVVideoPlayer::seek(int framenumber_new){
    // Constrains frame number within first and last frame numbers.
    int targetFrame = qMin(qMax(0, framenumber_new), videoLength-1);
    cap->set(CAP_PROP_POS_FRAMES, targetFrame);
    //Update the playback window with a "preview" of the current frame
    cap->read(*frameIn);
    frameUpdate();
    // Move playhead back to where it was prior to grabbing a preview frame
    cap->set(CAP_PROP_POS_FRAMES, targetFrame);

    // Only emit the change signal if frame number actually changes. Otherwise,
    // still go through the motions, but don't emit the change signal.
    if(targetFrame != frameNumber){
        frameNumber = targetFrame;
        emit positionChanged(frameNumber);
    }
}

void OpenCVVideoPlayer::seek_ms(qreal msec){
    if(frameInterval != 0.0){
        seek(qRound(msec/frameInterval));
    }
    else {
        seek(0);
    }
}

void OpenCVVideoPlayer::jog(int frames){
    seek(frameNumber + frames);
}

int OpenCVVideoPlayer::getCurrentFrame(){
    return frameNumber;
}

bool OpenCVVideoPlayer::isPlaying(){
    return playing;
}

double OpenCVVideoPlayer::getFPS(){
    return fps;
}

double OpenCVVideoPlayer::getFrameInterval(){
    return frameInterval;
}

int OpenCVVideoPlayer::getTotalFrames(){
    return videoLength;
}

int OpenCVVideoPlayer::getTimeMs(){
    return int(frameNumber*frameInterval);
}

int OpenCVVideoPlayer::getDurationMs(){
    return int((videoLength - 1)*frameInterval);
}

void OpenCVVideoPlayer::frameUpdate(){
    frameNumber = int(cap->get(CAP_PROP_POS_FRAMES));
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
    if(!frameIn->empty())
        imshow(*frameIn);
}

void OpenCVVideoPlayer::release(){
    cap->release();
}

bool OpenCVVideoPlayer::atEnd(){
    return frameNumber >= videoLength-1;
}

void OpenCVVideoPlayer::syncCap(){
    seek(int(cap->get(CAP_PROP_POS_FRAMES)));
}

QString OpenCVVideoPlayer::formatTime(qreal seconds){
    int hr = abs(int(seconds/3600));
    int min = abs((int(seconds)-(hr*3600))/60);
    int sec = abs(int(seconds)-(hr*3600)-(min*60));
    int ms = abs(int(1000*(seconds - int(seconds))));
    return QString("%1%2:%3:%4.%5").arg((seconds<0)?"-":"")
            .arg(hr, 2, 10, QChar('0'))
            .arg(min, 2, 10, QChar('0'))
            .arg(sec, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));
}

bool OpenCVVideoPlayer::advanceFrame(){
    bool result = cap->read(*frameIn);
    if(!result){
        playing = false;
        qWarning() << "Had trouble getting frame";
    }
    return result;
}
