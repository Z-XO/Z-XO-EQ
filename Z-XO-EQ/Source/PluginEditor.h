/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/

// Construction of Slider 
struct CustomRotatarySlider : juce::Slider {

    CustomRotatarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox) {

    }

};

class ZXOEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ZXOEQAudioProcessorEditor (ZXOEQAudioProcessor&);
    ~ZXOEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ZXOEQAudioProcessor& audioProcessor;

    // Adding Sliders to private class

    CustomRotatarySlider parametricFrequencySlider;
    CustomRotatarySlider parametricGainSlider;
    CustomRotatarySlider parametricQualitySlider;

    CustomRotatarySlider lowCutFrequencySlider;
    CustomRotatarySlider highCutFrequencySlider;

    CustomRotatarySlider lowCutSlopeSlider;
    CustomRotatarySlider highCutSlopeSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZXOEQAudioProcessorEditor)
};
