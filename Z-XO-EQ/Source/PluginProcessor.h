/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <array>
#include <JuceHeader.h>


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




template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                numSamples,
                false,   //clear everything?
                true,    //including the extra space?
                true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
            "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

enum Channel
{
    RightChannel, //effectively 0
    LeftChannel //effectively 1
};


template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }

    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1,             //channel
            bufferSize,    //num samples
            false,         //keepExistingContent
            true,          //clear extra space
            true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
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


using Coefficients = Filter::CoefficientsPtr;

void updateCoefficients(Coefficients& old, const Coefficients& replacements);

Coefficients makeParametricFilter(const ChainParameters& chainParameters, double sampleRate);


inline auto makeLowCutFilter(const ChainParameters& chainParameters, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainParameters.lowCutFrequency,
        sampleRate,
        (2 * (chainParameters.lowCutSlope + 1)));
}

inline auto makeHighCutFilter(const ChainParameters& chainParameters, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainParameters.highCutFrequency,
        sampleRate,
        (2 * (chainParameters.highCutSlope + 1)));
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


    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo{ Channel::LeftChannel };
    SingleChannelSampleFifo<BlockType> rightChannelFifo{ Channel::RightChannel };

private:
    //==============================================================================


    MonoChain left;

    MonoChain right;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZXOEQAudioProcessor)
};
