/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ZXOEQAudioProcessor::ZXOEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

ZXOEQAudioProcessor::~ZXOEQAudioProcessor()
{
}

//==============================================================================
const juce::String ZXOEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ZXOEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ZXOEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ZXOEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ZXOEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ZXOEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ZXOEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ZXOEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ZXOEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void ZXOEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ZXOEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    left.prepare(spec);
    right.prepare(spec);


    auto chainParameters = getChainParameters(state);

    auto parametricCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 
        chainParameters.parametricFrequency, 
        chainParameters.parametricQuality,
        juce::Decibels::decibelsToGain(chainParameters.parametricGain));


    *left.get<ChainLocations::Parametric>().coefficients = *parametricCoeffs;
    *right.get<ChainLocations::Parametric>().coefficients = *parametricCoeffs;

    // Slope Choice for low cut coefficients
    // Slope choice of 0 corresponds to 12 dB per octave translating to an order of 2
    // Slope choice of 1 corresponds to 24 dB per octave translating to an order of 4, etc...
    
    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainParameters.lowCutFrequency, 
        sampleRate, 
        (2 * (chainParameters.lowCutSlope + 1)));

    // Left audio chain initialization -> lowCut filter chain
    
    auto& leftLowCut = left.get<ChainLocations::LowCut>();
 

    // bypass positions in chain

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);
   // leftLowCut.setBypassed<4>(true); <-- array out of index issue

    switch (chainParameters.lowCutSlope) {

    case Slope_12dB:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        break;
    }

    case Slope_24dB:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        break;
    }



    case Slope_36dB: {

        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        break;
    }


    case Slope_48dB: {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        leftLowCut.setBypassed<3>(false);
        break;
    
    }
    }

    auto& rightLowCut = right.get<ChainLocations::LowCut>();

    // bypass positions in chain

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);
    // leftLowCut.setBypassed<4>(true); <-- array out of index issue

    switch (chainParameters.lowCutSlope) {

    case Slope_12dB:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        break;
    }

    case Slope_24dB:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        break;
    }



    case Slope_36dB: {

        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        break;
    }


    case Slope_48dB: {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        rightLowCut.setBypassed<3>(false);
        break;

    }
    }

    // FOR HIGH CUT ( LOW PASS HIGH ORDER)
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainParameters.highCutFrequency,
        sampleRate,
        (2 * (chainParameters.highCutSlope + 1)));

    auto& leftHighCut = left.get<ChainLocations::HighCut>();

    leftHighCut.setBypassed<0>(true);
    leftHighCut.setBypassed<1>(true);
    leftHighCut.setBypassed<2>(true);
    leftHighCut.setBypassed<3>(true);


    switch (chainParameters.highCutSlope) {

    case Slope_12dB:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        break;
    }

    case Slope_24dB:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        break;
    }



    case Slope_36dB: {

        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        break;
    }


    case Slope_48dB: {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        *leftHighCut.get<3>().coefficients = *highCutCoefficients[3];
        leftHighCut.setBypassed<3>(false);
        break;

    }
    }

    auto& rightHighCut = right.get<ChainLocations::HighCut>();

    rightHighCut.setBypassed<0>(true);
    rightHighCut.setBypassed<1>(true);
    rightHighCut.setBypassed<2>(true);
    rightHighCut.setBypassed<3>(true);


    switch (chainParameters.highCutSlope) {

    case Slope_12dB:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        break;
    }

    case Slope_24dB:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        break;
    }



    case Slope_36dB: {

        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        break;
    }


    case Slope_48dB: {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        *rightHighCut.get<3>().coefficients = *highCutCoefficients[3];
        rightHighCut.setBypassed<3>(false);
        break;

    }
    }

}

void ZXOEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ZXOEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ZXOEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    //for (int channel = 0; channel < totalNumInputChannels; ++channel)
    //{
    //    auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    // }
    

    auto chainParameters = getChainParameters(state);

    auto parametricCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(),
        chainParameters.parametricFrequency,
        chainParameters.parametricQuality,
        juce::Decibels::decibelsToGain(chainParameters.parametricGain));


    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainParameters.lowCutFrequency,
        getSampleRate(),
        (2 * (chainParameters.lowCutSlope + 1)));

    // Left audio chain initialization -> lowCut filter chain

    auto& leftLowCut = left.get<ChainLocations::LowCut>();

    // bypass positions in chain

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);
    // leftLowCut.setBypassed<4>(true); <-- array out of index issue

    switch (chainParameters.lowCutSlope) {

    case Slope_12dB:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        break;
    }

    case Slope_24dB:
    {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        break;
    }



    case Slope_36dB: {

        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        break;
    }


    case Slope_48dB: {
        *leftLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        leftLowCut.setBypassed<3>(false);
        break;

    }
    }

    auto& rightLowCut = right.get<ChainLocations::LowCut>();

    // bypass positions in chain

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);
    // leftLowCut.setBypassed<4>(true); <-- array out of index issue

    switch (chainParameters.lowCutSlope) {

    case Slope_12dB:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        break;
    }

    case Slope_24dB:
    {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        break;
    }



    case Slope_36dB: {

        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        break;
    }


    case Slope_48dB: {
        *rightLowCut.get<0>().coefficients = *lowCutCoefficients[0];
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *lowCutCoefficients[1];
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *lowCutCoefficients[2];
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *lowCutCoefficients[3];
        rightLowCut.setBypassed<3>(false);
        break;

    }
    }

    // FOR HIGH CUT ( LOW PASS HIGH ORDER)
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainParameters.highCutFrequency,
        getSampleRate(),
        (2 * (chainParameters.highCutSlope + 1)));

    auto& leftHighCut = left.get<ChainLocations::HighCut>();

    leftHighCut.setBypassed<0>(true);
    leftHighCut.setBypassed<1>(true);
    leftHighCut.setBypassed<2>(true);
    leftHighCut.setBypassed<3>(true);


    switch (chainParameters.highCutSlope) {

    case Slope_12dB:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        break;
    }

    case Slope_24dB:
    {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        break;
    }



    case Slope_36dB: {

        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        break;
    }


    case Slope_48dB: {
        *leftHighCut.get<0>().coefficients = *highCutCoefficients[0];
        leftHighCut.setBypassed<0>(false);
        *leftHighCut.get<1>().coefficients = *highCutCoefficients[1];
        leftHighCut.setBypassed<1>(false);
        *leftHighCut.get<2>().coefficients = *highCutCoefficients[2];
        leftHighCut.setBypassed<2>(false);
        *leftHighCut.get<3>().coefficients = *highCutCoefficients[3];
        leftHighCut.setBypassed<3>(false);
        break;

    }
    }

    auto& rightHighCut = right.get<ChainLocations::HighCut>();

    rightHighCut.setBypassed<0>(true);
    rightHighCut.setBypassed<1>(true);
    rightHighCut.setBypassed<2>(true);
    rightHighCut.setBypassed<3>(true);


    switch (chainParameters.highCutSlope) {

    case Slope_12dB:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        break;
    }

    case Slope_24dB:
    {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        break;
    }



    case Slope_36dB: {

        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        break;
    }


    case Slope_48dB: {
        *rightHighCut.get<0>().coefficients = *highCutCoefficients[0];
        rightHighCut.setBypassed<0>(false);
        *rightHighCut.get<1>().coefficients = *highCutCoefficients[1];
        rightHighCut.setBypassed<1>(false);
        *rightHighCut.get<2>().coefficients = *highCutCoefficients[2];
        rightHighCut.setBypassed<2>(false);
        *rightHighCut.get<3>().coefficients = *highCutCoefficients[3];
        rightHighCut.setBypassed<3>(false);
        break;

    }
    }

    *left.get<ChainLocations::Parametric>().coefficients = *parametricCoeffs;
    *right.get<ChainLocations::Parametric>().coefficients = *parametricCoeffs;
    
    
    
    juce::dsp::AudioBlock<float> block(buffer);

    auto leftB = block.getSingleChannelBlock(0);
    auto rightB = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftB);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightB);

    left.process(leftContext);
    right.process(rightContext);


}

//==============================================================================
bool ZXOEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ZXOEQAudioProcessor::createEditor()
{
    //return new juce::GenericAudioProcessorEditor(*this);
    return new ZXOEQAudioProcessorEditor (*this);
}

//==============================================================================
void ZXOEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ZXOEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}


ChainParameters getChainParameters(juce::AudioProcessorValueTreeState& state) {

    ChainParameters parameters;

    parameters.lowCutFrequency = state.getRawParameterValue("LowCut Frequency")->load();
    parameters.lowCutSlope = static_cast<SlopeValues>(state.getRawParameterValue("LowCut Slope")->load());
    parameters.highCutFrequency = state.getRawParameterValue("HighCut Frequency")->load();
    parameters.highCutSlope = static_cast<SlopeValues>(state.getRawParameterValue("HighCut Slope")->load());
    parameters.parametricFrequency = state.getRawParameterValue("Parametric Frequency")->load();
    parameters.parametricGain = state.getRawParameterValue("Parametric Gain")->load();
    parameters.parametricQuality = state.getRawParameterValue("Parametric Quality")->load();


    return parameters;
}

juce::AudioProcessorValueTreeState::ParameterLayout ZXOEQAudioProcessor::createParameterLayout() {
  

    juce::AudioProcessorValueTreeState::ParameterLayout layout;


    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Frequency", "LowCut Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Frequency", "HighCut Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Parametric Frequency", "Parametric Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 750.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Parametric Gain", "Parametric Gain", juce::NormalisableRange<float>(-30.f, 30.f, 0.25f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Parametric Quality", "Parametric Quality", juce::NormalisableRange<float>(0.1f, 15.f, 0.05f, 1.f), 1.f));


    juce::StringArray values;

    for (auto i = 0; i < 4; ++i) {

        juce::String string;
        string << (12 + (i * 12));
        string << "dB per Octave";
        values.add(string);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", values, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", values, 0));


    return layout;
}



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZXOEQAudioProcessor();
}
