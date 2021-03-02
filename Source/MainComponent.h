#pragma once

#include <JuceHeader.h>
#include "Dynamic1DWave.h"
#include "Global.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent, public Timer, public Slider::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    double limit (double val); // limiter for your ears
    
    void timerCallback() override;
    
    void sliderValueChanged (Slider* slider) override;

private:
    //==============================================================================
    // Your private member variables go here...
    double fs;
    unsigned long n = 0;
    std::unique_ptr<Dynamic1DWave> dynamic1DWave;
    
    std::vector<double> cVec;
    Slider waveSpeedSlider;
    
//    std::vector<std::shared_ptr<std::ofstream>> files;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
