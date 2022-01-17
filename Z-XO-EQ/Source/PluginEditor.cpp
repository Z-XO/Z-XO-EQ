/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float roataryStartAngle, float rotaryEndAngle, juce::Slider& slider) {


    auto bounds = juce::Rectangle<float>(x, y, width, height);

    auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = roataryStartAngle + sliderPosProportional * (rotaryEndAngle - roataryStartAngle);

    // fill
    g.setColour(juce::Colours::steelblue);
    g.fillEllipse(rx, ry, rw, rw);

    // outline
    g.setColour(juce::Colours::ghostwhite);
    g.drawEllipse(rx, ry, rw, rw, 2.0f);

    juce::Rectangle<float> r;

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*> (&slider) ) {
        
        g.setFont(rswl->getTextBoxHeight());
        auto text = rswl->getDisplayString();
        auto stringWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(stringWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        g.setColour(juce::Colours::steelblue);
        g.fillRect(r);

        g.setColour(juce::Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
  
    
    }


    juce::Path p;
    auto pointerLength = radius * 0.53f;
    auto pointerThickness = 2.0f;
    p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    // pointer
    g.setColour(juce::Colours::yellow);
    g.fillPath(p);

}

juce::String RotarySliderWithLabels::getDisplayString() const {
    if (auto* choiceParameter = dynamic_cast<juce::AudioParameterChoice*>(audioParam)) {
        
        return choiceParameter->getCurrentChoiceName();

    }

    juce::String string;
    bool addK = false;

    if (auto* floatParameter = dynamic_cast<juce::AudioParameterFloat*>(audioParam)) {
        float value = getValue();

        if (value > 999.f) {
            value /= 1000.f;
            addK = true;

        }
        
        string = juce::String(value, (addK ? 2 : 0));
    }
    if (suffix.isNotEmpty()) {
        
        string << " ";
        if (addK == true) {
            string << "k";

            string << suffix;
        }
    }

    return string;
}

void RotarySliderWithLabels::paint(juce::Graphics& g) {

    auto startingRotaryLocation = juce::degreesToRadians(180.f + 45.f);
    auto endingRotaryLocation = juce::degreesToRadians(180.f - 45.f) + juce::MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBoundary = getSliderBounds();


    getLookAndFeel().drawRotarySlider(g, sliderBoundary.getX(), 
        sliderBoundary.getY(), sliderBoundary.getWidth(), sliderBoundary.getHeight(),
        juce::jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), startingRotaryLocation, endingRotaryLocation, *this);
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
    
   // return getLocalBounds();
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;

    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;

}


ResponseCurveComponent::ResponseCurveComponent(ZXOEQAudioProcessor& p) : audioProcessor(p) {
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters) {
        parameter->addListener(this);
    }

    startTimerHz(165);

}

ResponseCurveComponent::~ResponseCurveComponent() {
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters) {
        parameter->removeListener(this);
    }


}
void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {

    shouldUpdateParameters.set(true);
}

void ResponseCurveComponent::timerCallback() {

    if (shouldUpdateParameters.compareAndSetBool(false, true)) {

        auto chainParameters = getChainParameters(audioProcessor.state);
        auto parametricCoefficients = makeParametricFilter(chainParameters, audioProcessor.getSampleRate());

        updateCoefficients(MonoChain.get<ChainLocations::Parametric>().coefficients, parametricCoefficients);


        auto lowCutCoefficients = makeLowCutFilter(chainParameters, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainParameters, audioProcessor.getSampleRate());

        updateCutFilter(MonoChain.get<ChainLocations::LowCut>(), lowCutCoefficients, chainParameters.lowCutSlope);
        updateCutFilter(MonoChain.get<ChainLocations::HighCut>(), highCutCoefficients, chainParameters.highCutSlope);

        repaint();

    }
}

    void ResponseCurveComponent::paint (juce::Graphics & g){
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll(juce::Colours::grey);

        
        auto visualResponse = getLocalBounds();
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

        g.setColour(juce::Colours::ghostwhite);
        g.drawRoundedRectangle(visualResponse.toFloat(), 4.f, 4.f);

        g.setColour(juce::Colours::black);
        g.strokePath(responseCurve, juce::PathStrokeType(4.f));


    }




//==============================================================================
    ZXOEQAudioProcessorEditor::ZXOEQAudioProcessorEditor(ZXOEQAudioProcessor& p)
        : AudioProcessorEditor(&p), audioProcessor(p), 
        parametricFrequencySlider(*audioProcessor.state.getParameter("Parametric Frequency"), "Hz"),
        parametricGainSlider(*audioProcessor.state.getParameter("Parametric Gain"), "dB"),
        parametricQualitySlider(*audioProcessor.state.getParameter("Parametric Quality"), ""),
        lowCutFrequencySlider(*audioProcessor.state.getParameter("LowCut Frequency"), "Hz"),
        lowCutSlopeSlider(*audioProcessor.state.getParameter("LowCut Slope"), "dB/Oct"),
        highCutFrequencySlider(*audioProcessor.state.getParameter("HighCut Frequency"), "Hz"),
        highCutSlopeSlider(*audioProcessor.state.getParameter("HighCut Slope"), "dB/Oct"),
        responseCurveComponent(audioProcessor),




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

    addAndMakeVisible(responseCurveComponent);

    setSize (600, 800);
}

ZXOEQAudioProcessorEditor::~ZXOEQAudioProcessorEditor()
{
    
}



void ZXOEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Half of GUI dedicated to visual response
    auto visualResponse = bounds.removeFromTop(bounds.getHeight() * 0.50);

    responseCurveComponent.setBounds(visualResponse);
    
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


