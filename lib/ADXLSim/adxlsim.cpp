#include "adxlsim.h"

ADXLSim2::ADXLSim2(double threshAct, double threshInact, int timeAct, int timeInact, QObject *parent):QObject(parent){
    reset();
    this->threshAct = threshAct;
    this->threshInact = threshInact;
    this->timeAct = timeAct;
    this->timeInact = timeInact;
}


/**
 * @brief ADXLSim::next Performs one cycle of the algorithm, taking one sample from the
 * signal source.
 */
void ADXLSim2::next(double X, double Y, double Z){
    /*
     * Assuming operation in loop mode, so the ADXL is only either awake or not awake.
     *
     * To go inactive, the ADXL must be "awake" and see at least [timeInact] consecutive
     * samples where [maxDeltaInact] is below [threshInact].
     *
     * Conversely, to go active, the ADXL must not be "awake" and see at least [timeAct]
     * consecutive samples where [maxDeltaAct] is above [threshAct].
     *
     * The ADXL starts in the inactive or not "awake" state.
     *
     * [maxDeltaAct] and [maxDeltaInact] are calculated by taking the maximum of the absolute
     * value of the difference between each axis accelerometer sample and a "baseline" or
     * "reference" value for each axis. These reference values are set when:
     *
     *  1. Measurement mode is entered for the first time (i.e. when this simulator is started
     *      for the first time): Both active and inactive references are reset to zero.
     *  2. Inactive reference is set to the current accelerometer sample when the the delta of
     *      any sample and its inactive reference (i.e. [maxDeltaInact]) exceeds [threshInact]
     *  3. Active reference is set to the current sample value upon entering the "inactive" or
     *      not "awake" state
     */
    double current[3] = {X, Y, Z};
    double maxDeltaInact = qMax(qMax(qAbs(current[ACCEL_X] - refInact[ACCEL_X]),
                                     qAbs(current[ACCEL_Y] - refInact[ACCEL_Y])),
                                     qAbs(current[ACCEL_Z] - refInact[ACCEL_Z]));

    double maxDeltaAct = qMax(qMax(qAbs(current[ACCEL_X] - refAct[ACCEL_X]),
                                   qAbs(current[ACCEL_Y] - refAct[ACCEL_Y])),
                                   qAbs(current[ACCEL_Z] - refAct[ACCEL_Z]));

    if(awake){
        if(maxDeltaInact < threshInact){
            ++countInact;
            if(countInact >= timeInact){
                awake = false;
                resetRefActive = true;
                countInact = 0;
            }
        }
        else{ // If sample exceeds inactive threshold, re-reference the inactive threshold to current sample.
            countInact = 0;
            memcpy(refInact, current, 3*sizeof(refInact[0]));
        }
    }
    else{
        // If sample exceeds active threshold continuously for [countAct] samples, transition to awake state.
        if(maxDeltaAct > threshAct){
            ++countAct;
            if(countAct >= timeAct){
                awake = true;
                countAct = 0;
            }
        }
        else{
            countAct = 0;
        }
        if(maxDeltaInact >= threshInact)
            // Re-references the inactive threshold
            memcpy(refInact, current, 3*sizeof(refInact[0]));
        if(resetRefActive){
            // Re-references the active threshold only on the first inactive sample
            memcpy(refAct, current, 3*sizeof(refInact[0]));
            resetRefActive = false;
        }
    }
}

bool ADXLSim2::isActive(){
    return awake;
}

void ADXLSim2::reset(){
    awake = false;
    countAct = 0;
    countInact = 0;
    resetRefActive = false;
    std::fill_n(refAct, 3, 0);
    std::fill_n(refInact, 3, 0);
}
