#ifndef ACTIVITYDETECTOR_H
#define ACTIVITYDETECTOR_H

#include <QObject>
#include "timeseries.h"

/**
 * @brief The ActivityDetector class provides an interface for implementing your own activity detectors.
 * Activity detectors are designed to handle streaming data and thus must be able to keep their own internal state.
 */

class ActivityDetector : public QObject
{
    Q_OBJECT
public:
    explicit ActivityDetector(QObject *parent = nullptr);

    /**
     * @brief config Set configuration options for your activity detector
     * @param config A key-value map of configuration options, provided by the user interface.
     * @return true if the configuration is valid, false otherwise (you must implement the check yourself)
     */
    virtual bool config(QMap<QString, qreal> config) = 0;

    /**
     * @brief next Runs an additional iteration of the activity detector.
     * @param sample A key-value map of a single sample, with QString labels for sensor values corresponding to the columns in the original data's CSV.
     * @return true if successful, false if something goes wrong. Call getErrorString() for more details.
     */
    virtual bool next(QMap<QString, qreal> sample) = 0;

    /**
     * @brief isActive Returns whether or not the activity detector is detecting activity for the current sample.
     * @return true if active, false otherwise.
     */
    virtual bool isActive() = 0;

    /**
     * @brief getErrorString Returns a description of the last-occurred error
     * @return The error string
     */
    virtual QString getErrorString() = 0;

signals:

public slots:
};

#endif // ACTIVITYDETECTOR_H
