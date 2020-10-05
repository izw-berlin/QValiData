#ifndef OPENCVDISPLAY_H
#define OPENCVDISPLAY_H

#include <QObject>
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <opencv2/opencv.hpp>

class OpenCVDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit OpenCVDisplay(QWidget *parent = nullptr);
    QPoint scaleCoordinate(QPoint coord);


protected:
    void paintEvent(QPaintEvent *event) override; // Rendering routine
    void resizeEvent(QResizeEvent *event) override;

    //Mouse event handlers...a lot of this is to replicate OpenCV's High GUI behavior (Along with some more fanciness)
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;


private:
    QImage *frame;
    // (Qt needs constant access to the raw pixel data or else it will segfault)
    cv::Mat buffer; // Stores a local copy of the OpenCV Mat
    QRect *placementRect;


    bool hasImage; // Remembers if an image has been loaded (Qt will also segfault if it tries to display a null image)

signals:
    // "Raw" mouse handlers: Returns the absolute position of mouse event relative
    // to current widget dimensions and positioning
    void mousePressed(QMouseEvent *event);
    void mouseMoved(QMouseEvent *event);

    // "Relative" mouse handlers: Returns the position of the mouse event relative
    // to the upper left corner of image, and the real image dimensions
    void mousePressedRelative(QMouseEvent *event);
    void mouseMovedRelative(QMouseEvent *event);

    void keyPressed(QKeyEvent *event);
    void entered(QEvent *event);
    void left(QEvent *event);

public slots:
    virtual void imshow(cv::Mat &img); // Replicates openCV's imshow routine

};

#endif // OPENCVDISPLAY_H
