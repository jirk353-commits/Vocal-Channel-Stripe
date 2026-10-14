#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

namespace vsui
{
using APVTS = juce::AudioProcessorValueTreeState;

namespace col
{
    const juce::Colour bgTop    { 0xff1b1d22 };
    const juce::Colour bgBot    { 0xff0b0c0f };
    const juce::Colour panelTop { 0xff1d2027 };
    const juce::Colour panelBot { 0xff121419 };
    const juce::Colour edge     { 0xff2e323c };
    const juce::Colour text     { 0xfff2f4f8 };
    const juce::Colour textDim  { 0xffa8afbd };
    const juce::Colour screen   { 0xff07080b };
    const juce::Colour gold     { 0xffffb347 };
}

/** module accent colours, in chain order */
juce::Colour moduleColour (int index);
juce::Font uiFont (float size, bool bold = false, float kerning = 0.0f);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (APVTS&, const juce::String& id, const juce::String& title, juce::Colour accent, bool big = false);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
private:
    juce::String title; bool big;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons (juce::RangedAudioParameter&, const juce::StringArray& labels, juce::Colour accent, int columns = 0);
    void resized() override;
    std::function<void (int)> onChange;
    int getSelected() const { return selected; }
private:
    void update (int);
    juce::OwnedArray<juce::TextButton> buttons;
    std::unique_ptr<juce::ParameterAttachment> att;
    int selected = -1, cols = 0;
};

class Toggle : public juce::TextButton
{
public:
    Toggle (APVTS&, const juce::String& id, const juce::String& text, juce::Colour accent);
private:
    std::unique_ptr<APVTS::ButtonAttachment> att;
};

//==============================================================================
/** Base for one module page: panel, title, description and the module's on/off. */
class Page : public juce::Component
{
public:
    Page (VocalStripProcessor&, int index, const juce::String& title, const juce::String& description, const juce::String& onId);
    void paint (juce::Graphics&) override;
    void resized() override;
    virtual void layout (juce::Rectangle<int> area) = 0;
    virtual void paintContent (juce::Graphics&) {}
    virtual void tick() {}                 // ~30 Hz from the main panel
protected:
    juce::Rectangle<int> box (juce::Rectangle<int> r, juce::Graphics& g, const juce::String& label = {}) const;
    VocalStripProcessor& proc;
    juce::Colour accent;
    juce::String title, description;
    std::unique_ptr<Toggle> power;
    juce::Rectangle<int> content;
};

//==============================================================================
class EqGraph : public juce::Component
{
public:
    explicit EqGraph (VocalStripProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void tick();
    std::function<void (int)> onBandSelected;
    int selected = 0;
private:
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float gainToY (float db) const;
    float yToGain (float y) const;
    juce::Point<float> nodePos (int band) const;
    int hitNode (juce::Point<float>) const;
    float band (int b, const char* what) const;
    void set (int b, const char* what, float v);

    VocalStripProcessor& proc;
    juce::dsp::FFT fft { 11 };
    std::vector<float> ring = std::vector<float> (2048, 0.0f), fftData = std::vector<float> (4096, 0.0f), spectrum = std::vector<float> (1024, -100.0f);
    int ringPos = 0;
    juce::Path curve;
    juce::String curveSig;
    int dragging = -1;
};

//==============================================================================
class MainPanel : public juce::Component, private juce::Timer
{
public:
    explicit MainPanel (VocalStripProcessor&);
    ~MainPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 1200, H = 740;
    void select (int page);
private:
    void timerCallback() override;
    juce::Rectangle<int> tileBounds (int i) const;

    VocalStripProcessor& proc;
    LookAndFeel lnf;
    juce::OwnedArray<Page> pages;
    juce::OwnedArray<Toggle> tilePower;
    Knob inGain, outGain;
    juce::ComboBox presetBox;
    juce::Image texture;
    int current = 0;
    float inLvl = -100, outLvl = -100, cGr = 0, dGr = 0, gGr = 0;
};
} // namespace vsui

class VocalStripEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalStripEditor (VocalStripProcessor&);
    void resized() override;
    void showPage (int page) { panel.select (page); }
private:
    vsui::MainPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalStripEditor)
};
