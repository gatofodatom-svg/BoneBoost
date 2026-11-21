#include "PluginEditor.h"

BoostEditor::BoostEditor(BoostProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
      sliderAtt(p.apvts, "boost", boostSlider),
      modeAtt(p.apvts, "mode", modeBox)
{
    boostSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    boostSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 24);
    boostSlider.setTextValueSuffix("%");
    boostSlider.setNumDecimalPlacesToDisplay(0);
    addAndMakeVisible(boostSlider);

    modeBox.addItem("Normal", 1);
    modeBox.addItem("Boost", 2);
    addAndMakeVisible(modeBox);

    setSize(420, 520);
    startTimerHz(60);
}

float BoostEditor::getAmount() const
{
    const bool boostOn = proc.apvts.getRawParameterValue("mode")->load() > 0.5f;
    if (!boostOn) return 0.f;
    return juce::jlimit(0.f, 1.f, proc.apvts.getRawParameterValue("boost")->load() / 100.f);
}

void BoostEditor::resized()
{
    boostSlider.setBounds(30, getHeight() - 95, getWidth() - 60, 30);
    modeBox.setBounds(getWidth() / 2 - 70, getHeight() - 50, 140, 28);
}

void BoostEditor::timerCallback()
{
    const float amt = getAmount();
    phase += 0.08f + 0.25f * amt;

    // chuva de bone a partir de 90%
    if (amt >= 0.9f)
    {
        const int tries = amt > 0.98f ? 3 : 1;
        for (int k = 0; k < tries; ++k)
            if (rng.nextFloat() < 0.6f)
                rain.push_back({rng.nextFloat() * (float) getWidth(), -30.f,
                                4.f + rng.nextFloat() * 5.f, rng.nextFloat() * 6.f,
                                (rng.nextFloat() - 0.5f) * 0.3f, rng.nextInt(5)});
    }
    for (auto& c : rain) { c.y += c.vy; c.rot += c.vr; }
    const float h = (float) getHeight();
    rain.erase(std::remove_if(rain.begin(), rain.end(),
                              [h](const FallingCap& c) { return c.y > h + 30.f; }),
               rain.end());
    repaint();
}

void BoostEditor::drawCap(juce::Graphics& g, float x, float y, float rot, int colourIndex)
{
    static const juce::Colour cols[] = {juce::Colour(0xffe63946), juce::Colour(0xff3a86ff),
                                        juce::Colour(0xffffbe0b), juce::Colour(0xff2ec4b6),
                                        juce::Colour(0xff9b5de5)};
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(rot, x, y));
    g.setColour(cols[colourIndex % 5]);
    juce::Path dome;
    dome.addCentredArc(x, y, 22.f, 22.f, 0.f, -juce::MathConstants<float>::halfPi,
                       juce::MathConstants<float>::halfPi, true);
    dome.closeSubPath();
    g.fillPath(dome);
    g.fillRoundedRectangle(x - 2.f, y - 3.f, 38.f, 6.f, 3.f);   // aba
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.fillEllipse(x - 3.f, y - 25.f, 6.f, 6.f);                 // botao
    g.restoreState();
}

void BoostEditor::paint(juce::Graphics& g)
{
    const float amt = getAmount();
    const float shake = amt > 0.8f ? (amt - 0.8f) / 0.2f : 0.f;

    g.fillAll(juce::Colour(0xff15151f).interpolatedWith(juce::Colour(0xff6a1030), amt * 0.6f));

    g.saveState();
    g.addTransform(juce::AffineTransform::translation((rng.nextFloat() - 0.5f) * 16.f * shake,
                                                      (rng.nextFloat() - 0.5f) * 16.f * shake));

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(22.f, juce::Font::bold));
    g.drawText("BONE BOOST", 0, 12, getWidth(), 30, juce::Justification::centred);

    const float cx = getWidth() * 0.5f;
    const float neckY = (float) getHeight() - 190.f;

    // corpo
    g.setColour(juce::Colour(0xffff8c42));
    g.fillRoundedRectangle(cx - 38.f, neckY, 76.f, 85.f, 22.f);

    // cabeca balancando (headbang)
    const float bang = juce::jlimit(0.f, 1.f, (amt - 0.25f) / 0.5f);
    const float angle = std::sin(phase) * 0.4f * bang;
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(angle, cx, neckY));

    const float headY = neckY - 42.f;
    g.setColour(juce::Colour(0xffffd2a6));
    g.fillEllipse(cx - 42.f, headY - 42.f, 84.f, 84.f);

    // olhos: abertos no comeco, fechados curtindo depois
    g.setColour(juce::Colours::black);
    for (float ex : {cx - 16.f, cx + 16.f})
    {
        if (amt > 0.25f)
        {
            juce::Path p;
            p.startNewSubPath(ex - 8.f, headY - 2.f);
            p.quadraticTo(ex, headY - 12.f, ex + 8.f, headY - 2.f);
            g.strokePath(p, juce::PathStrokeType(3.f));
        }
        else
            g.fillEllipse(ex - 4.f, headY - 8.f, 8.f, 10.f);
    }
    // boca
    if (amt > 0.7f)
        g.fillEllipse(cx - 10.f, headY + 12.f, 20.f, 8.f + 14.f * (amt - 0.7f) / 0.3f);
    else
    {
        juce::Path m;
        m.startNewSubPath(cx - 12.f, headY + 14.f);
        m.quadraticTo(cx, headY + 14.f + 14.f * amt, cx + 12.f, headY + 14.f);
        g.strokePath(m, juce::PathStrokeType(3.f));
    }

    // bones empilhados: mais boost = mais bone
    const int n = amt <= 0.f ? 0 : juce::jlimit(1, 8, (int) std::ceil(amt * 8.f));
    const float topY = headY - 38.f;
    for (int i = 0; i < n; ++i)
        drawCap(g, cx + ((i % 2) ? 5.f : -5.f) - 6.f, topY + 4.f - i * 13.f,
                ((i % 3) - 1) * 0.15f, i);

    g.restoreState();

    // chuva de bones
    for (auto& c : rain)
        drawCap(g, c.x, c.y, c.rot, c.col);

    g.restoreState();
}
