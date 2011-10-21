#pragma once

class adcSamples : public dataSamples {
public:
    adcSamples() : dataSamples(1134) {}

    adcSamples*
    isAdc() { return this; }
};
