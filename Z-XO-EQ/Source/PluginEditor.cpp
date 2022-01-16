/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ZXOEQAudioProcessorEditor::ZXOEQAudioProcessorEditor (ZXOEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
 
    addAndMakeVisible(parametricFrequencySlider);
    addAndMakeVisible(parametricGainSlider);
    addAndMakeVisible(parametricQualitySlider);

    addAndMakeVisible(lowCutFrequencySlider);
    addAndMakeVisible(highCutFrequencySlider);

    addAndMakeVisible(lowCutSlopeSlider);
    addAndMakeVisible(highCutSlopeSlider);




    setSize (600, 800);
}

ZXOEQAudioProcessorEditor::~ZXOEQAudioProcessorEditor()
{
}

//==============================================================================
void ZXOEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);

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
