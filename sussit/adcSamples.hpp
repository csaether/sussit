#pragma once

class adcSamples : public dataSamples {
public:
    adcSamples() : dataSamples(700) {}

    adcSamples*
    isAdc() { return this; }
};
