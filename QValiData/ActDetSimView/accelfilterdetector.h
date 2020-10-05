#ifndef ACCELFILTERDETECTOR_H
#define ACCELFILTERDETECTOR_H
#include "activitydetector.h"
#include "Iir.h"

#define FILTER_ORDER 4

class AccelFilterDetector : public ActivityDetector
{
public:
    AccelFilterDetector();

    bool config(QMap<QString, qreal> config) override;
    bool next(QMap<QString, qreal> sample) override;
    bool isActive() override;
    QString getErrorString() override;

private:
    Iir::Butterworth::HighPass<FILTER_ORDER> highPassX, highPassY, highPassZ;
    int holdTimeSet;
    int holdTimeCounter;
    int delayTimeSet;
    int delayTimeCounter;
    bool active;
    double thresh;
    double xPrev, yPrev, zPrev;
    QString errorString;
};

#endif // ACCELFILTERDETECTOR_H
