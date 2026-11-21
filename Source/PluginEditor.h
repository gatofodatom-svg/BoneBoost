#pragma once
#include "PluginProcessor.h"

class BoostEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BoostEditor(BoostProcessor&);
    ~BoostEditor() override { stopTimer(); }

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    float getAmount() const;
    void drawCap(juce::Graphics&, float x, float y, float rot, int colourIndex);

    BoostProcessor& proc;
    juce::Slider boostSlider;
    juce::ComboBox modeBox;
    juce::AudioProcessorValueTreeState::SliderAttachment sliderAtt;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment modeAtt;

    struct FallingCap { float x, y, vy, rot, vr; int col; };
    std::vector<FallingCap> rain;
    float phase = 0.f;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoostEditor)
};
