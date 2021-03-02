/*
  ==============================================================================

    Dynamic1DWave.h
    Created: 18 Feb 2021 2:33:27pm
    Author:  Silvin Willemsen

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Global.h"
//==============================================================================
/*
*/
class Dynamic1DWave  : public juce::Component
{
public:
    Dynamic1DWave (NamedValueSet& parameters, double k);
    ~Dynamic1DWave() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    Path visualiseState (double visualScaling, Graphics& g);
    void calculate();
    
    void recalculateCoeffs();
    void addRemovePoint();

    void calculateInterpolatedPoints();
    void lowPassConnection();
    
    void calculateScheme();
    void displacementCorrection();

    void updateStates();
    
    double getOutput (double ratio) { int idx = floor(Nint * ratio);
        if (idx <= M)
            return u[1][idx];
        else
            return w[1][idx-M-1];
    };
    
    void excite();
    
    void changeWavespeed (double val) { cToUse = val; }; // c is only used once per sample (before everything else)
    void updateParams() { c = cToUse; };
    
    void saveToFiles();
    void closeFiles();
private:
    double k;        // One over the samplerate
    int Nint, NintPrev, M, Mw; // integer number of points
    
    double N, c, lambdaSq, h, L;
    
    double cToUse;
    
    double alf, alfTick;
    
    // states u
    std::vector<std::vector<double>> uStates;
    std::vector<double*> u;
    
    // states w
    std::vector<std::vector<double>> wStates;
    std::vector<double*> w;

    // virtual grid points used to calculate inner boundaries
    double uMp1, wm1;
    std::vector<double> quadIp;
    std::vector<double> customIp;
    
    double lpExponent = 10;
    
    std::ofstream uState, wState, alfSave, MSave, MwSave;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dynamic1DWave)
};
