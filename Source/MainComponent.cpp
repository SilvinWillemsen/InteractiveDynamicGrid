#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    Timer::stopTimer();
    shutdownAudio();

}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    fs = sampleRate;

    if (Global::useCVec)
        cVec = Global::linspace(294, 588, fs);
    
    NamedValueSet parameters;
    parameters.set ("c", Global::useCVec ? cVec[0] : 600);
    parameters.set ("L", 1);
    
    dynamic1DWave = std::make_unique<Dynamic1DWave>(parameters, 1.0 / fs);
    double test = static_cast<double>(*parameters.getVarPointer("L")) * fs / (Global::maxN);
    waveSpeedSlider.setRange (test, 2000.0);
    waveSpeedSlider.setValue (*parameters.getVarPointer("c"));
    waveSpeedSlider.addListener (this);

    Timer::startTimerHz (15);
    setSize (800, 600);
    
    addAndMakeVisible (waveSpeedSlider);
    addAndMakeVisible (dynamic1DWave.get());


}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
    
    // Get pointers to output locations
    float* const channelData1 = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
    float* const channelData2 = bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample);
    
    float output = 0.0;
    
    // only update params once a buffer
//    dynamic1DWave->updateParams();
    
    for (int i = 0; i < bufferToFill.numSamples; ++i)
    {

        if (Global::useCVec && n < cVec.size())
            dynamic1DWave->changeWavespeed(cVec[n]);
        
        dynamic1DWave->updateParams();
        dynamic1DWave->calculate();
//        if (n < 22050)
//        {
//            dynamic1DWave->saveToFiles();
//        }
//        else
//        {
//            std::cout << "done" << std::endl;
//            dynamic1DWave->closeFiles();
//        }
        dynamic1DWave->updateStates();
        
        output = dynamic1DWave->getOutput (0.2); // get output at 0.8L of the string
//        std::cout << output << std::endl;
        channelData1[i] = limit (output);
        channelData2[i] = limit (output);
        
        ++n;
    }
}


void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    Rectangle<int> totArea = getLocalBounds();
    waveSpeedSlider.setBounds (totArea.removeFromBottom(Global::sliderHeight));
    dynamic1DWave->setBounds (totArea);
}

// limiter
double MainComponent::limit (double val)
{
    if (val < -1)
    {
        val = -1;
        return val;
    }
    else if (val > 1)
    {
        val = 1;
        return val;
    }
    return val;
}

void MainComponent::timerCallback()
{
    repaint();
}

void MainComponent::sliderValueChanged (Slider* slider)
{
    if (slider == &waveSpeedSlider && !Global::useCVec)
        dynamic1DWave->changeWavespeed (slider->getValue());
}
