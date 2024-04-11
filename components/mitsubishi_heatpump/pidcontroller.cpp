#include "pidcontroller.h"

#include <cmath>

#include "espmhp.h"
using namespace esphome;
using namespace std;

PIDController::PIDController(
    float p,
    float i,
    float d,
    int sampleTime,
    float target,
    float outputMin,
    float outputMax
): p(p),
   i(i),
   d(d),
   outputSum(0.0f)
{
    this->sampleTime = sampleTime;
    this->setTunings();
    this->setOutputLimits(outputMin, outputMax);
    this->setTarget(target);
}

float PIDController::applyOutputLimits(float output) {
    if (output > this->outputMax) {
        return this->outputMax;
    }
    if (output < this->outputMin) {
        return this->outputMin;
    }
    return output;
}

float PIDController::update(float input) {
    if (!this->lastInput.has_value()) {
        this->lastInput = input;
    }

    if (this->outputSum != this->outputSum) {
        this->outputSum = 0.0f;
    }

    //ESP_LOGI(TAG, "PIDController update:");
    //ESP_LOGI(TAG, "  lastInput:          %f", this->lastInput.value());
    
    //ESP_LOGI(TAG, "  target:             %f", this->target);
    //ESP_LOGI(TAG, "  input:              %f", input);
    float error = this->target - input;
    //ESP_LOGI(TAG, "  error:              %f", error);
    
    float inputDelta = input - this->lastInput.value();
    //ESP_LOGI(TAG, "  inputDelta:         %f", inputDelta);

    //ESP_LOGI(TAG, "  ki:                 %f", this->ki);
    //ESP_LOGI(TAG, "  outputSum:          %f", this->outputSum);
    this->outputSum += (this->ki * error);
    //ESP_LOGI(TAG, "  outputSum:          %f", this->outputSum);
    this->outputSum = this->applyOutputLimits(this->outputSum);
    //ESP_LOGI(TAG, "  outputSum:          %f", this->outputSum);

    //ESP_LOGI(TAG, "  kp:                 %f", this->kp);
    float output = this->kp * error;
    //ESP_LOGI(TAG, "  output:             %f", output);

    //ESP_LOGI(TAG, "  kd:                 %f", this->kd);
    output += this->outputSum - this->kd * inputDelta;
    //ESP_LOGI(TAG, "  output:             %f", output);
    this->output = this->applyOutputLimits(output);
    //ESP_LOGI(TAG, "  output:             %f", output);

    // Update state variables
    this->lastInput = input;
    //ESP_LOGI(TAG, "  lastInput:          %f", this->lastInput.value());
    //ESP_LOGI(TAG, "  output:             %f", this->output);
    return this->output;
}

void PIDController::setOutputLimits(float min, float max) {
    if (min > max) {
        return;//throw new Error('Minimum output limit cannot be bigger than maximum');
    }
    this->outputMin = min;
    this->outputMax = max;
    // Apply to current state
    this->output = this->applyOutputLimits(this->output);
    this->outputSum = this->applyOutputLimits(this->outputSum);
}

float roundToDecimals(const float value, const int n) {
    const int decimals = std::pow(10, n);
    return std::ceil(value * decimals) / decimals;
}

void PIDController::setTunings() {
//    if (p < 0 || i < 0 || d < 0) {
//        return;//throw new Error('PID values cannot be below 0');
//    }

    // Calculate internal pid representations
    float sampleTimeSec = this->sampleTime / 1000.0;
    float sampleTimeSecRounded = roundToDecimals(sampleTimeSec, 1000000);
    this->kp = this->p;
    this->ki = this->i * sampleTimeSecRounded;
    this->kd = this->d / sampleTimeSecRounded;
}

float PIDController::getOutputMin() {
    return this->outputMin;
}

float PIDController::getOutputMax() {
    return this->outputMax;
}

float PIDController::getTarget() {
    return this->target;
}

void PIDController::setTarget(float target) {
    this->target = target;
    this->resetState();
}

void PIDController::resetState() {
    this->outputSum = this->applyOutputLimits(0.0f);
    this->lastInput.reset();
}

void PIDController::dumpConfig() {
    ESP_LOGI(TAG, "PIDController config:");
    ESP_LOGI(TAG, "  p:          %f", this->p);
    ESP_LOGI(TAG, "  i:          %f", this->i);
    ESP_LOGI(TAG, "  d:          %f", this->d);
    ESP_LOGI(TAG, "  target:     %f", this->target);
    ESP_LOGI(TAG, "  sampleTime: %d", this->sampleTime);
    ESP_LOGI(TAG, "  outputMin:  %f", this->outputMin);
    ESP_LOGI(TAG, "  outputMax:  %f", this->outputMax);
    ESP_LOGI(TAG, "  outputSum:  %f", this->outputSum);
    ESP_LOGI(TAG, "  lastInput:  %f", this->lastInput.value_or(-1.0));
}