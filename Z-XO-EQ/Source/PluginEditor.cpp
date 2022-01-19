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
        string = juce::String(value);
    }

    if (suffix.contains("dB")) {
        
        string < " dB";

        string << suffix;
    }

    if (suffix.contains("Hz")) {

        string < " Hz";

        string << suffix;
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


    auto center = sliderBoundary.toFloat().getCentre();
    auto radius = sliderBoundary.getWidth() * 0.5f;

    g.setColour(juce::Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();

    for (auto i = 0; i < numChoices; ++i) {

        auto position = labels[i].positions;
        jassert(0.f <= position);
        jassert(position <= 1.f);

        auto angle = juce::jmap(position, 0.f, 1.f, startingRotaryLocation, endingRotaryLocation);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * .5f, angle);

        juce::Rectangle<float> r;
        auto string = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(string), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        
        g.drawFittedText(string, r.toNearestInt(), juce::Justification::centred, 1);
    }
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

ResponseCurveComponent::ResponseCurveComponent(ZXOEQAudioProcessor& p) : 
    audioProcessor(p),
leftChannelFifo(&audioProcessor.leftChannelFifo) {
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters) {
        parameter->addListener(this);
    }

    leftChannelFFTDataGenerator.changeOrder(FFTOrder::order8192);
    monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());

    updateChain();

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


    juce::AudioBuffer<float> temporaryIncomingBuffer;

    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0) {
        
        
        if (leftChannelFifo->getAudioBuffer(temporaryIncomingBuffer)) {
            auto size = temporaryIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0), monoBuffer.getReadPointer(0, size), monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size), temporaryIncomingBuffer.getReadPointer(0, 0), size);

            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -100.f);
        }
    }

    // bin width = 48000 / 8192 = 5.85hz
    
    const auto fftBounds = getAnalysisArea().toFloat();
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = audioProcessor.getSampleRate() / (double)fftSize;


    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0) {

        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData)) {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -100.f);
        }

    }

    while (pathProducer.getNumPathsAvailable()) {

        pathProducer.getPath(LeftChannelFFTPath);
    }



    if (shouldUpdateParameters.compareAndSetBool(false, true)) {

        updateChain();


    }
    
    repaint();
}

void ResponseCurveComponent::updateChain(){

    auto chainParameters = getChainParameters(audioProcessor.state);
    auto parametricCoefficients = makeParametricFilter(chainParameters, audioProcessor.getSampleRate());

    updateCoefficients(MonoChain.get<ChainLocations::Parametric>().coefficients, parametricCoefficients);


    auto lowCutCoefficients = makeLowCutFilter(chainParameters, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainParameters, audioProcessor.getSampleRate());

    updateCutFilter(MonoChain.get<ChainLocations::LowCut>(), lowCutCoefficients, chainParameters.lowCutSlope);
    updateCutFilter(MonoChain.get<ChainLocations::HighCut>(), highCutCoefficients, chainParameters.highCutSlope);
}

void ResponseCurveComponent::paint (juce::Graphics & g){
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll(juce::Colours::grey);

        g.drawImage(background, getLocalBounds().toFloat());


        auto visualResponse = getAnalysisArea();

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

        auto xAxis = visualResponse.getX() - 6;
        auto yAxis = visualResponse.getY() - 43;

       // LeftChannelFFTPath.applyTransform(juce::AffineTransform().translation(JUCE_LIVE_CONSTANT(5),
        //    JUCE_LIVE_CONSTANT(5)));
        
        // LeftChannelFFTPath.applyTransform(juce::AffineTransform().translation(visualResponse.getX(), visualResponse.getY()));


       g.setColour(juce::Colours::yellow);
       g.strokePath(LeftChannelFFTPath, juce::PathStrokeType(1.f));
   
        
        g.setColour(juce::Colours::ghostwhite);
        g.drawRoundedRectangle(getAnalysisArea().toFloat(), 2.f, 2.f);

        g.setColour(juce::Colours::yellow);
        g.strokePath(responseCurve, juce::PathStrokeType(2.5f));



    }


void ResponseCurveComponent::resized() {

  

        background = juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true);

        juce::Graphics g(background);

        juce::Array<float> frequencies{ 
        
        20,  50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000
        
        
        };

        auto renderArea = getRenderArea();
        auto left = renderArea.getX();
        auto right = renderArea.getRight();
        auto top = renderArea.getY();
        auto bottom = renderArea.getBottom();
        auto width = renderArea.getWidth();

        juce::Array < float > xs;

        for (auto freqs : frequencies) {
            auto normX = juce::mapFromLog10(freqs, 20.f, 20000.f);
            xs.add(left + width * normX);
        }



        g.setColour(juce::Colours::ghostwhite);

        for (auto x : xs) {
           // auto normX = juce::mapFromLog10(freqs, 20.f, 20000.f);

          //  g.drawVerticalLine(getWidth() * normX, 0.f, getHeight());
            g.drawVerticalLine(x, top, bottom);
        }

        juce::Array<float> gains{

        -30, -24, -18, -12, -6, 0, 6, 12, 18, 24, 30

        };

        for (auto gain : gains) {
            auto normY = juce::jmap(gain, -30.f, 30.f, float(bottom), float(top));

          //  g.drawHorizontalLine(normY, 0, getWidth());
            g.setColour(gain == 0 ? juce::Colour(0u, 172u, 1u) : juce::Colours::grey);
            g.drawHorizontalLine(normY, left, right);
        }

        g.setColour(juce::Colours::yellow);
        const int fontHeight = 12;
        g.setFont(fontHeight);

        for (auto j = 0; j < frequencies.size(); ++j) {

            auto freqs = frequencies[j];
            auto x = xs[j];


            bool addK = false;
            juce::String string;

            if (freqs > 999.f) 
            {
                addK = true;
                freqs /= 1000.f;
            }

            string << freqs;
            if (addK == true) 
            {
                string << "k";
            }

            string << "Hz";

            auto textWidth = g.getCurrentFont().getStringWidth(string);
            
            
            juce::Rectangle<int> r;
            r.setSize(textWidth, fontHeight);
            r.setCentre(x, 0);
            r.setY(1);

            g.drawFittedText(string, r, juce::Justification::centred, 1);
        }


        const int dbFont = 8;
        for (auto gain : gains) {
            auto normY = juce::jmap(gain, -30.f, 30.f, float(bottom), float(top));

            juce::String string;
            if (gain > 0) {
                string << "+";

            }
            string << gain << "dB";
       
            auto textWidth = g.getCurrentFont().getStringWidth(string);


            juce::Rectangle<int> r;
            r.setSize(textWidth, dbFont);
            r.setX(getWidth() - textWidth - 1);
            r.setCentre(r.getCentreX(), normY);

            g.setColour(gain == 0 ? juce::Colour(0u, 172u, 1u) : juce::Colours::yellow);
            g.drawFittedText(string, r, juce::Justification::centred, 1);



   

        }


    }

juce::Rectangle<int> ResponseCurveComponent::getRenderArea() {

        auto bounds = getAnalysisArea();
        bounds.removeFromTop(10);
        bounds.removeFromBottom(10);

        return bounds;



    }

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea() {

        auto bounds = getLocalBounds();

         //bounds.reduce(JUCE_LIVE_CONSTANT(5),
         //    JUCE_LIVE_CONSTANT(5));

        //bounds.reduce(30, 15);

        return bounds;
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
 
        parametricFrequencySlider.labels.add({ 0.f , "20Hz" });
        parametricFrequencySlider.labels.add({ 1.f , "20kHz" });

        parametricGainSlider.labels.add({ 0.f , "-30dB" });
        parametricGainSlider.labels.add({ 1.f , "30dB" });

        parametricQualitySlider.labels.add({ 0.f , "0.1" });
        parametricQualitySlider.labels.add({ 1.f , "15" });

        lowCutFrequencySlider.labels.add({ 0.f , "20Hz" });
        lowCutFrequencySlider.labels.add({ 1.f , "20kHz" });

        highCutFrequencySlider.labels.add({ 0.f , "20Hz" });
        highCutFrequencySlider.labels.add({ 1.f , "20kHz" });

        lowCutSlopeSlider.labels.add({ 0.f , "12 dB/Oct" });
        lowCutSlopeSlider.labels.add({ 1.f , "48 dB/Oct" });

        highCutSlopeSlider.labels.add({ 0.f , "12 dB/Oct" });
        highCutSlopeSlider.labels.add({ 1.f , "48 dB/Oct" });


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


