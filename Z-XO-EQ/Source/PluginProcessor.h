/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


// enumeration of slope values

enum SlopeValues {

    Slope_12dB,
    Slope_24dB,
    Slope_36dB,
    Slope_48dB
};

struct ChainParameters {

    float parametricFrequency{ 0 };
    float parametricGain{ 0 };
    float parametricQuality{ 1.f };
    float lowCutFrequency{ 0 };
    float highCutFrequency{ 0 };

    SlopeValues  lowCutSlope{ SlopeValues::Slope_12dB };
    SlopeValues  highCutSlope{ SlopeValues::Slope_12dB };

};




ChainParameters getChainParameters(juce::AudioProcessorValueTreeState& state);

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using MonoChain = juce::dsp::ProcessorChain < CutFilter, Filter, CutFilter >;


enum ChainLocations {

    LowCut,
    Parametric,
    HighCut
};

// <---------------------------------------------------------------------->

using Coefficients = Filter::CoefficientsPtr;

void updateCoefficients(Coefficients& old, const Coefficients& replacements);

Coefficients makeParametricFilter(const ChainParameters& chainParameters, double sampleRate);


inline auto makeLowCutFilter(const ChainParameters& chainParameters, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainParameters.lowCutFrequency,
        sampleRate,
        (2 * (chainParameters.lowCutSlope + 1)));
}

inline auto makeHighCutFilter(const ChainParameters& chainParameters, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainParameters.lowCutFrequency,
        sampleRate,
        (2 * (chainParameters.lowCutSlope + 1)));
}

template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& chain,
    const CoefficientType& coefficients,
    const SlopeValues& slope)
{
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);

    switch (slope)
    {
    case Slope_48dB:
    {
        update<3>(chain, coefficients);
    }
    case Slope_36dB:
    {
        update<2>(chain, coefficients);
    }
    case Slope_24dB:
    {
        update<1>(chain, coefficients);
    }
    case Slope_12dB:
    {
        update<0>(chain, coefficients);
    }
    }
}



// <------------------------------------------------------------------------>

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

    void updateParametricFilter(const ChainParameters& chainParameters);
    void updateLowCutFilters(const ChainParameters& chainParameters);
    void updateHighCutFilters(const ChainParameters& chainParameters);


    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();


    juce::AudioProcessorValueTreeState state {*this, nullptr, "Parameters", createParameterLayout()};

private:
    //==============================================================================


    MonoChain left;

    MonoChain right;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZXOEQAudioProcessor)
};
