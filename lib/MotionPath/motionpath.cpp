#include "motionpath.h"

MotionPath::MotionPath()
{
    start = 0;
    end = 0;
    path = new QMap<int, QPoint>();
}

MotionPath::MotionPath(int start, int end){
    this->start = start;
    this->end = end;
    path = new QMap<int, QPoint>();
    for(int i=start; i<end; ++i){
        path->insert(i, QPoint(0, 0));
    }
}

void MotionPath::putPoint(int frame, QPoint point){
    if(path->size() == 0){
        start = frame;
        end = frame;
    }
    else{
        if(frame < start)
            start = frame;
        if(frame > end)
            end = frame;
    }
    path->insert(frame, point);
}

QPoint MotionPath::getPoint(int frame){
    return path->value(frame);
}

QList<QPoint> MotionPath::getPoints(){
    return path->values();
}

bool MotionPath::contains(int frame){
    return path->contains(frame);
}

QString MotionPath::toString(){
    return QString("(Start: %1, End: %2)").arg(start).arg(end);
}
