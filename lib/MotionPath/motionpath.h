#ifndef MOTIONPATH_H
#define MOTIONPATH_H

#include <QList>
#include <QMap>
#include <QString>

class MotionPath
{
public:
    MotionPath();
    MotionPath(int start, int end);
    void putPoint(int frame, QPoint point);
    QPoint getPoint(int frame);
    QList<QPoint> getPoints();
    bool contains(int frame);
    QString toString();
    int start;
    int end;

private:
    QMap<int, QPoint> *path;
};

#endif // MOTIONPATH_H
