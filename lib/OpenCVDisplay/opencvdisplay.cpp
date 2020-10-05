#include "opencvdisplay.h"
#include <QEvent>
#include <QPainter>
#include <QDebug>

OpenCVDisplay::OpenCVDisplay(QWidget *parent) : QWidget(parent)
{
    frame = new QImage(this->size(), QImage::Format_RGB888);
    placementRect = new QRect(this->rect());
    hasImage = false;
    setMouseTracking(true);
}

void OpenCVDisplay::imshow(cv::Mat &img){
    // OpenCV works in BGR, Qt works in RGB, so we convert pixel data.
    cvtColor(img, buffer, cv::COLOR_BGR2RGB);

    // Bring the Mat into Qt's realm by transferring its data to a QImage object.
    frame = new QImage(buffer.data, buffer.cols, buffer.rows, int(buffer.step), QImage::Format_RGB888);
    hasImage = true;
    update();
}
void OpenCVDisplay::paintEvent(QPaintEvent *event){
    QPainter painter(this);
    if(hasImage){
        // Scale image to keep aspect ratio
        QImage imgScaled(frame->scaled(QSize(this->width(), this->height()), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // Center image on canvas
        placementRect = new QRect(imgScaled.rect());
        placementRect->moveCenter(this->rect().center());

        // Draw image
        painter.drawImage(placementRect->topLeft(), imgScaled);
    }
    //Draw a black screen if no image has been given.
    else{
        painter.fillRect(QRect(0, 0, this->width(), this->height()), QBrush(Qt::black));
    }
    event->accept();
}

// Re-maps coordinate onto the original image, accounting for scale and placement.
QPoint OpenCVDisplay::scaleCoordinate(QPoint coord){
    qreal scaleFactor;
    if(frame->width() != 0)
        scaleFactor = double(placementRect->width()) / double(frame->width());
    else
        scaleFactor = 1;
    return QPoint(int((coord.x()-placementRect->topLeft().x())/scaleFactor),
                  int((coord.y()-placementRect->topLeft().y())/scaleFactor));
}

void OpenCVDisplay::resizeEvent(QResizeEvent *event){
    Q_UNUSED(event)
}

//Emits both a regular mouse press event and a relative mouse press event.
void OpenCVDisplay::mousePressEvent(QMouseEvent *event){
    emit mousePressed(event);
    emit mousePressedRelative(new QMouseEvent(event->type(),
                                            scaleCoordinate(event->pos()),
                                            event->button(), event->buttons(),
                                            event->modifiers()));
    event->accept();
}

void OpenCVDisplay::mouseMoveEvent(QMouseEvent *event){
    emit mouseMoved(event);
    emit mouseMovedRelative(new QMouseEvent(event->type(),
                                            scaleCoordinate(event->pos()),
                                            event->button(), event->buttons(),
                                            event->modifiers()));
    event->accept();
}

void OpenCVDisplay::keyPressEvent(QKeyEvent *event){
    emit keyPressed(event);
    event->accept();
}

void OpenCVDisplay::enterEvent(QEvent *event){
    emit entered(event);
    event->accept();
}
void OpenCVDisplay::leaveEvent(QEvent *event){
    emit left(event);
    event->accept();
}
