/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct ChainParameters {

    float parametricFrequency{ 0 };
    float parametricGain{ 0 };
    float parametricQuality{ 1.f };
    float lowCutFrequency{ 0 };
    float highCutFrequency{ 0 };

    int lowCutSlope{ 0 };
    int highCutSlope{ 0 };

};


ChainParameters getChainParameters(juce::AudioProcessorValueTreeState& state);

//==============================================================================
/**
*/
class ZXOEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ZXOEQAudioProcessor();
    ~ZXOEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();


    juce::AudioProcessorValueTreeState state {*this, nullptr, "Parameters", createParameterLayout()};

private:
    //==============================================================================

    using Filter = juce::dsp::IIR::Filter<float>;

    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    using MonoChain = juce::dsp::ProcessorChain < CutFilter, Filter, CutFilter >;

    MonoChain left;

    MonoChain right;

    enum ChainLocations {

        LowCut,
        Parametric,
        HighCut
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZXOEQAudioProcessor)
};
