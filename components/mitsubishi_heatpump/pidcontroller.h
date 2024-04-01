// p, i, d, sampleTime, target, outputMin, outputMax, proportionalOnMeasurement = false

#include "esphome.h"
using namespace esphome;

#ifndef PIDC_H
#define PIDC_H

float roundToDecimals(const float value, const int n);

/*
    Based on: @mariusrumpf/pid-controller
*/
class PIDController {
    private:
        const float p;
        const float i;
        const float d;

        float kp;
        float ki;
        float kd;
        float outputMin;
        float outputMax;

        float output;
        float outputSum = 0.0f;
        float target;
        optional<float> lastInput;
        int sampleTime;

        void setTunings();
        void setOutputLimits(float outputMin, float outputMax);

        float applyOutputLimits(float output);

    public:
        PIDController(
            float p,
            float i,
            float d,
            int sampleTime,
            float target,
            float outputMin,
            float outputMax
        );

        float getOutputMin();
        float getOutputMax();

        float getTarget();
        void setTarget(float target);
        float update(float input);
        void resetState();
        void setSampleTime(int sampleTime);

        void dumpConfig();
};

#endif