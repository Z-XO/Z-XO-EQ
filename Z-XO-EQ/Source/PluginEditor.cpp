/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#include "PluginProcessor.cpp"
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ZXOEQAudioProcessorEditor::ZXOEQAudioProcessorEditor (ZXOEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    parametricFrequencySliderAttachment(audioProcessor.state, "Parametric Frequency", parametricFrequencySlider),
    parametricGainSliderAttachment(audioProcessor.state, "Parametric Gain", parametricGainSlider),
    parametricQualitySliderAttachment(audioProcessor.state, "Parametric Quality", parametricQualitySlider),
    lowCutFrequencySliderAttachment(audioProcessor.state, "LowCut Frequency", lowCutFrequencySlider),
    lowCutSlopeSliderAttachment(audioProcessor.state, "LowCut Slope", lowCutSlopeSlider),
    highCutFrequencySliderAttachment(audioProcessor.state, "HighCut Frequency", highCutFrequencySlider),
    highCutSlopeSliderAttachment(audioProcessor.state, "HighCut Slope", highCutSlopeSlider)
{
 
    addAndMakeVisible(parametricFrequencySlider);
    addAndMakeVisible(parametricGainSlider);
    addAndMakeVisible(parametricQualitySlider);

    addAndMakeVisible(lowCutFrequencySlider);
    addAndMakeVisible(highCutFrequencySlider);

    addAndMakeVisible(lowCutSlopeSlider);
    addAndMakeVisible(highCutSlopeSlider);

    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters) {
        parameter->addListener(this);
    }

    startTimerHz(60);

    setSize (600, 800);
}

ZXOEQAudioProcessorEditor::~ZXOEQAudioProcessorEditor()
{
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters) {
        parameter->removeListener(this);
    }

}

//==============================================================================
void ZXOEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::grey);

    auto bounds = getLocalBounds();
    auto visualResponse = bounds.removeFromTop(bounds.getHeight() * 0.50);
    auto width = visualResponse.getWidth();

    auto& lowCutChain = MonoChain.get<ChainLocations::LowCut>();
    auto& highCutChain = MonoChain.get<ChainLocations::HighCut>();
    auto& parametricChain = MonoChain.get<ChainLocations::Parametric>();

    auto sampleRate = audioProcessor.getSampleRate();

   

    std::vector<double> magnitudes;
    magnitudes.resize(width);

    for (auto i = 0; i < width; ++i) {
        double magnitude = 1.f;
        auto frequency = juce::mapToLog10(double(i) / double(width), 20.0, 20000.0);


        if (!MonoChain.isBypassed<ChainLocations::Parametric>()) {
            magnitude *= parametricChain.coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }

        if (!lowCutChain.isBypassed<0>()) {
            magnitude *= lowCutChain.get<0>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
        if (!lowCutChain.isBypassed<1>()) {
            magnitude *= lowCutChain.get<1>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
        if (!lowCutChain.isBypassed<2>()) {
            magnitude *= lowCutChain.get<2>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
        if (!lowCutChain.isBypassed<3>()) {
            magnitude *= lowCutChain.get<3>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }

        if (!highCutChain.isBypassed<0>()) {
            magnitude *= highCutChain.get<0>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
        if (!highCutChain.isBypassed<1>()) {
            magnitude *= highCutChain.get<1>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
        if (!highCutChain.isBypassed<2>()) {
            magnitude *= highCutChain.get<2>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
        if (!highCutChain.isBypassed<3>()) {
            magnitude *= highCutChain.get<3>().coefficients->getMagnitudeForFrequency(frequency, sampleRate);
        }
    
        magnitudes[i] = juce::Decibels::gainToDecibels(magnitude);
    
    
    
    }

    juce::Path responseCurve;

    const double min = visualResponse.getBottom();
    const double max = visualResponse.getY();

    auto map = [min, max](double input) {
        return juce::jmap(input, -30.0, 30.0, min, max);
    };

    responseCurve.startNewSubPath(visualResponse.getX(), map(magnitudes.front()));

    for (size_t x = 1; x < magnitudes.size(); ++x) {
        responseCurve.lineTo(visualResponse.getX() + x, map(magnitudes[x]));
    }

    g.setColour(juce::Colours::orange);
    g.drawRoundedRectangle(visualResponse.toFloat(), 4.f, 1.f);

    g.setColour(juce::Colours::white);
    g.strokePath(responseCurve, juce::PathStrokeType(2.f));


}

void ZXOEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Half of GUI dedicated to visual response
    auto visualResponse = bounds.removeFromTop(bounds.getHeight() * 0.50);

    
    // Remaining half dedicated to nobs for low/high cut and parametric

    //LowCut, Left side of lower half
    auto lowCutLocation = bounds.removeFromLeft(bounds.getWidth() * 0.33);

    // HighCut, Right side of lower half
    auto highCutLocation = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowCutFrequencySlider.setBounds(lowCutLocation.removeFromTop(lowCutLocation.getHeight() * .50));
    lowCutSlopeSlider.setBounds(lowCutLocation);
    highCutFrequencySlider.setBounds(highCutLocation.removeFromTop(highCutLocation.getHeight() * .50));
    highCutSlopeSlider.setBounds(highCutLocation);
    
    // Remaining middle for Parametric EQ
    
    parametricFrequencySlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    parametricGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.50));
    parametricQualitySlider.setBounds(bounds);

}

void ZXOEQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue) {

    shouldUpdateParameters.set(true);
}

void ZXOEQAudioProcessorEditor::timerCallback() {

    if (shouldUpdateParameters.compareAndSetBool(false, true)) {
        
        auto chainParameters = getChainParameters(audioProcessor.state);
        auto parametricCoefficients = makeParametricFilter(chainParameters, audioProcessor.getSampleRate());

        updateCoefficients(MonoChain.get<ChainLocations::Parametric>().coefficients, parametricCoefficients);

        repaint();

    }
}
