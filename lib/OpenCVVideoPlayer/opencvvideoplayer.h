#ifndef OPENCVVIDEOPLAYER_H
#define OPENCVVIDEOPLAYER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include "opencvdisplay.h"

class OpenCVVideoPlayer : public OpenCVDisplay
{
    Q_OBJECT
public:
    explicit OpenCVVideoPlayer(QWidget *parent = nullptr);

    //If you have a video capture device open elsewhere, just attach to the existing one.
    void attachCap(cv::VideoCapture *cap);

    //Open a video file. Returns true and prepare for play if open successful, returns false if open unsuccessful
    bool openVideoFile(QString f);

    //Start playback from current location.
    virtual void play();

    //Temporarily stop playback and hold at current location.
    virtual void pause();

    //Jumps video to nth frame, or first frame (if negative), or the last frame (if framenumber provided is greater)
    virtual void seek(int framenumber_new);

    //Jumps video to a certain time stamp.
    virtual void seek_ms(qreal msec);

    //Increments/decrements, but never goes past first/last frame
    void jog(int frames);

    //Gets the current frame.
    int getCurrentFrame();

    //Gets whether or not the video player is playing.
    bool isPlaying();

    //Returns the total number of frames
    int getTotalFrames();

    //Get frame rate
    double getFPS();

    //Get timestamp in milliseconds
    int getTimeMs();

    //Get duration in milliseconds
    int getDurationMs();

    double getFrameInterval();

    void release();

    void currentFrame(cv::Mat *out);

    bool atEnd();

    static QString formatTime(qreal seconds);

    virtual bool advanceFrame();

protected:
    cv::Mat *frameIn;
    QTimer *frameTimer;
    cv::VideoCapture *cap;
    int frameNumber;
    int videoLength;
    bool playing;
    double fps;
    double frameInterval; // Time between frames, in ms

    // Get capture device info
    void getCapProperties();
signals:
    //Emits whenever the frame count is changed.
    void positionChanged(int frame);

    //Emits whenever the video player changes play state
    void playStateChanged(bool state);

public slots:
    //Advances frame if playing, or stops timer if not.
    virtual void frameUpdate();

    //If the capture device is linked from elsewhere, synchronize play position.
    virtual void syncCap();
};

#endif // OPENCVVIDEOPLAYER_H
