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


struct LookAndFeel : juce::LookAndFeel_V4 {

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
};

struct RotarySliderWithLabels : juce::Slider {

    RotarySliderWithLabels(juce::RangedAudioParameter& value, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox), audioParam(&value), suffix(unitSuffix) {

        setLookAndFeel(&LAF);
    }

    ~RotarySliderWithLabels() {
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;

private:

    LookAndFeel LAF;

    juce::RangedAudioParameter* audioParam;
    juce::String suffix;


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
    
    RotarySliderWithLabels parametricFrequencySlider;
    RotarySliderWithLabels parametricGainSlider;
    RotarySliderWithLabels parametricQualitySlider;

    RotarySliderWithLabels lowCutFrequencySlider;
    RotarySliderWithLabels highCutFrequencySlider;

    RotarySliderWithLabels lowCutSlopeSlider;
    RotarySliderWithLabels highCutSlopeSlider;

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
