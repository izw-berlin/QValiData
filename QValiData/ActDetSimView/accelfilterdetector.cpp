#include "accelfilterdetector.h"
#include "Iir.h"
/**
 * @brief AccelFilterDetector::AccelFilterDetector Emulates a bio-logger equipped with 3-axis accelerometer and a high pass fourth order Butterworth Filter.
 * The energy expenditure of a single movement roughly correlates with the frequency of its accelerations, so this activity detector can be used to detect
 * high-energy movements, such as flight in birds.
 */
AccelFilterDetector::AccelFilterDetector()
{
    active = false;
}

/**
 * @brief AccelFilterDetector::config Sets up the activity detector
 * @param config A key-value map that stores configuration data. For the Accel Filter Activity Detector, required values are:
 *      1. "samplerate": Sample Rate, in Hertz, of source data
 *      2. "cutoff": Cutoff frequency of the filter, which is lowest frequency that still retains half its power after passing through the filter.
 *      3. "thresh": Acceleration threshold for triggering "Active" state
 *      4. "delaytime": Number of seconds of continuous activity exceeding the threshold before transitioning to "Active"
 *      5. "holdtime": Number of seconds to keep an "Active" state when there is no activity. Small values can group several short activity events into a single large event, reducing the number of wakeups.
 * @return
 */
bool AccelFilterDetector::config(QMap<QString, qreal> config){
    if(!config.contains("samplerate") || !config.contains("cutoff") || !config.contains("holdtime") || !config.contains("thresh") || !config.contains("delaytime"))
        return false; // Invalid configuration if certain parameters are not specified

    const double samplerate = config.value("samplerate");
    const double cutoff = config.value("cutoff");
    thresh = config.value("thresh");
    holdTimeSet = int(samplerate * config.value("holdtime"));
    holdTimeCounter = holdTimeSet;

    delayTimeSet = int(samplerate * config.value("delaytime"));
    delayTimeCounter = delayTimeSet;

    // cutoff frequency must be at most half of the sample rate or Nyquist frequency
    if (cutoff > (samplerate * 0.5) || cutoff == 0)
        return false;

    highPassX.setup(samplerate, cutoff);
    highPassY.setup(samplerate, cutoff);
    highPassZ.setup(samplerate, cutoff);

    xPrev = 0;
    yPrev = 0;
    zPrev = 0;
    return true;
}
bool AccelFilterDetector::next(QMap<QString, qreal> sample){
    /*
     * Activity Detector Algorithm:
     *  1. Defaults to "Inactive"
     *  2. When any of the three axes (after filtering) exceeds the acceleration threshold for more than the "delay time", state is now "Active"
     *  3. When none of the three axes exeed the threshold, the "hold time" counter begins counting down.
     *  4. Hold Time counter continues counting down unless threshold is exceeded, which resets timer.
     *  5. When Hold Time counter reaches zero, state is once again "Inactive"
     */

    double filteredX,filteredY,filteredZ;
    if(!sample.contains("X") || !sample.contains("Y") || !sample.contains("Z")){
        errorString = "No valid data with labels \"X\", \"Y\", and \"Z\" received";
        return false;
    }
    filteredX = highPassX.filter(sample.value("X"));
    filteredY = highPassX.filter(sample.value("Y"));
    filteredZ = highPassX.filter(sample.value("Z"));

    double maxFiltered = qMax(filteredX, qMax(filteredY, filteredZ));

    if (maxFiltered > thresh){
        if(delayTimeCounter == 0){
            active = true;
        }
        else{
            delayTimeCounter --;
        }
        holdTimeCounter = holdTimeSet;
    }
    else{
        if(holdTimeCounter == 0){
            active = false;
        }
        else{
            holdTimeCounter --;
        }
        delayTimeCounter = delayTimeSet;
    }
    return true;
}

bool AccelFilterDetector::isActive(){
    return active;
}

QString AccelFilterDetector::getErrorString(){
    return errorString;
}
