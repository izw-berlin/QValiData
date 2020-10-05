#ifndef CLOSEUPVIDEOVIEWER_H
#define CLOSEUPVIDEOVIEWER_H

#include <QObject>
#include <QWidget>
#include <QTime>
#include <motionpath.h>
#include <opencvvideoplayer.h>

#define ZOOMFACTOR 3
#define INSETFRACTION 0.3
class CloseupVideoViewer : public OpenCVVideoPlayer
{
    Q_OBJECT
public:
    explicit CloseupVideoViewer(QWidget *parent = nullptr);
    void attachPath(QList<MotionPath *> *paths);
    void setMagnify(bool magnify);
private:
    QList<MotionPath *> *paths;
    MotionPath *currentPath;

    bool showMagnifier;

    QPoint thePoint;

public slots:
    void frameUpdate() override;
    virtual void imshow(cv::Mat &img) override;
};

#endif // CLOSEUPVIDEOVIEWER_H
