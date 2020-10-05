#ifndef ADXLSIM2_H
#define ADXLSIM2_H

#include <QObject>

/**
 * @brief The ADXLSim2 class: Basically the same in operation as ADXLSim, but more flexible, and operates in units of g, rather than raw binary values.
 */
enum AccelCh{ACCEL_X=0, ACCEL_Y=1, ACCEL_Z=2};
class ADXLSim2 : public QObject
{
    Q_OBJECT
public:
    explicit ADXLSim2(double threshAct, double threshInact, int timeAct, int timeInact, QObject *parent = nullptr);
    void next(double X, double Y, double Z);
    bool isActive();
    void reset();

private:
    bool awake;
    double refAct[3];       // Store the reference points for each axis to have a baseline for comparison
    double refInact[3];     // Same here
    bool resetRefActive;    // True if we need to reset the reference value for active threshold.

    int countAct;           // Keeps track of active/inactive samples since first trigger.
    int countInact;

    // Activity detection configuration variables
    double threshAct;
    double threshInact;
    int timeAct;
    int timeInact;

signals:

public slots:
};

#endif // ADXLSIM2_H
