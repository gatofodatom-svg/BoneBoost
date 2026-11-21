#include "PluginProcessor.h"
#include "PluginEditor.h"

BoostProcessor::BoostProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BoostProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"boost", 1}, "Boost",
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Modo",
        juce::StringArray{"Normal", "Boost"}, 1));
    return layout;
}

void BoostProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{sampleRate, (juce::uint32) samplesPerBlock, 2};
    bassFilter.prepare(spec);
    bassFilter.reset();
    smoothedAmount.reset(sampleRate, 0.05);
    smoothedAmount.setCurrentAndTargetValue(0.f);
}

bool BoostProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void BoostProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float target = apvts.getRawParameterValue("boost")->load() / 100.f;
    const bool boostOn = apvts.getRawParameterValue("mode")->load() > 0.5f;

    // Modo Normal = audio passa limpo (amount vai para 0)
    smoothedAmount.setTargetValue(boostOn ? target : 0.f);
    const float a = smoothedAmount.skip(buffer.getNumSamples());
    if (a < 0.0001f)
        return;

    // 1) Grave: low shelf em 90 Hz, ate +12 dB
    *bassFilter.state = *Coeffs::makeLowShelf(getSampleRate(), 90.f, 0.7f,
                                              juce::Decibels::decibelsToGain(12.f * a));
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    bassFilter.process(ctx);

    // 2) Volume: ate +3 dB
    buffer.applyGain(juce::Decibels::decibelsToGain(3.f * a));

    // 3) Saturacao suave (deixa mais alto e evita estourar)
    const float drive = 1.f + 2.f * a;
    const float norm = 1.f / std::tanh(drive);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            d[i] = std::tanh(d[i] * drive) * norm;
    }
}

juce::AudioProcessorEditor* BoostProcessor::createEditor() { return new BoostEditor(*this); }

void BoostProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void BoostProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BoostProcessor(); }
