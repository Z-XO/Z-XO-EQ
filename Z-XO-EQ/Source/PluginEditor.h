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
enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13,
    order16384 = 14
};

template<typename BlockType>
struct FFTDataGenerator
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // first apply a windowing function to our data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);       // [1]

        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());  // [2]

        int numBins = (int)fftSize / 2;

        //normalize the fft values.
        for (int i = 0; i < numBins; ++i)
        {
            auto v = fftData[i];
            //            fftData[i] /= (float) numBins;
            if (!std::isinf(v) && !std::isnan(v))
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }

        //convert them to decibels
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
        juce::Rectangle<float> fftBounds,
        int fftSize,
        float binWidth,
        float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                negativeInfinity, 0.f,
                float(bottom + 10), top);
        };

        auto y = map(renderData[0]);

        //        jassert( !std::isnan(y) && !std::isinf(y) );
        if (std::isnan(y) || std::isinf(y))
            y = bottom;

        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

            //            jassert( !std::isnan(y) && !std::isinf(y) );

            if (!std::isnan(y) && !std::isinf(y))
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct LookAndFeel : juce::LookAndFeel_V4 {

    void drawRotarySlider(juce::Graphics &, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

struct RotarySliderWithLabels : juce::Slider {

    RotarySliderWithLabels(juce::RangedAudioParameter& value, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox), audioParam(&value), suffix(unitSuffix) {

        setLookAndFeel(&LAF);
    }

    ~RotarySliderWithLabels() {
        setLookAndFeel(nullptr);
    }

    struct LabelPositions {
        float positions;
        juce::String label;

    };

    juce::Array<LabelPositions> labels;

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
    
    void updateChain();

    void resized() override;



private:
    ZXOEQAudioProcessor& audioProcessor;
    juce::Atomic<bool> shouldUpdateParameters{ false };

    MonoChain MonoChain;

    juce::Image background;

    juce::Rectangle<int> getAnalysisArea();

    juce::Rectangle<int> getRenderArea();

    SingleChannelSampleFifo<ZXOEQAudioProcessor::BlockType>* leftChannelFifo;
    SingleChannelSampleFifo<ZXOEQAudioProcessor::BlockType>* rightChannelFifo;


    juce::AudioBuffer<float> monoBufferL;
    juce::AudioBuffer<float> monoBufferR;

    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    FFTDataGenerator<std::vector<float>> rightChannelFFTDataGenerator;


    AnalyzerPathGenerator<juce::Path> pathProducerL;
    AnalyzerPathGenerator<juce::Path> pathProducerR;

    juce::Path LeftChannelFFTPath;
    juce::Path RightChannelFFTPath;

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

    juce::ToggleButton lowCutBypassButton;
    juce::ToggleButton highCutBypassButton;
    juce::ToggleButton parametricBypassButton;
    juce::ToggleButton analyzerEnableButton;

    juce::AudioProcessorValueTreeState::ButtonAttachment lowCutBypassButtonAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment highCutBypassButtonAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment parametricBypassButtonAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment analyzerEnableButtonAttachment;

    LookAndFeel LookNF;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZXOEQAudioProcessorEditor)
};
