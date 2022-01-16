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

struct ResponseCurveComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer {
   
    ResponseCurveComponent(ZXOEQAudioProcessor&);
   
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }

    void timerCallback() override;

    void paint(juce::Graphics& g) override;

private:
    ZXOEQAudioProcessor& audioProcessor;
    juce::Atomic<bool> shouldUpdateParameters{ false };

    MonoChain MonoChain;


};

class ZXOEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ZXOEQAudioProcessorEditor (ZXOEQAudioProcessor&);
    ~ZXOEQAudioProcessorEditor() override;

    //==============================================================================
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ZXOEQAudioProcessor& audioProcessor;


    // Adding Sliders to private class

    ResponseCurveComponent responseCurveComponent;
    
    CustomRotatarySlider parametricFrequencySlider;
    CustomRotatarySlider parametricGainSlider;
    CustomRotatarySlider parametricQualitySlider;

    CustomRotatarySlider lowCutFrequencySlider;
    CustomRotatarySlider highCutFrequencySlider;

    CustomRotatarySlider lowCutSlopeSlider;
    CustomRotatarySlider highCutSlopeSlider;

    // Attach sliders to DSP values

    juce::AudioProcessorValueTreeState::SliderAttachment parametricFrequencySliderAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment parametricGainSliderAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment parametricQualitySliderAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment lowCutFrequencySliderAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment highCutFrequencySliderAttachment;

    juce::AudioProcessorValueTreeState::SliderAttachment lowCutSlopeSliderAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment highCutSlopeSliderAttachment;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZXOEQAudioProcessorEditor)
};
