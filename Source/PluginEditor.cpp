#include "PluginEditor.h"

using namespace juce;

namespace vsui
{
Colour moduleColour (int i)
{
    static const Colour c[] = { Colour (0xff4fd8ff),   // EQ
                                Colour (0xffffb347),   // COMP
                                Colour (0xffff6fa8),   // DE-ESS
                                Colour (0xffff7a45),   // SAT
                                Colour (0xffa98bff),   // GLUE
                                Colour (0xff7cf0b0),   // DELAY
                                Colour (0xff5b9dff) }; // REVERB
    return c[jlimit (0, 6, i)];
}

Font uiFont (float size, bool bold, float kerning)
{
    Font f { FontOptions (size, bold ? Font::bold : Font::plain) };
    return kerning > 0.0f ? f.withExtraKerningFactor (kerning) : f;
}

//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, Colour (0xff15171c));
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::gold.withAlpha (0.85f));
    setColour (PopupMenu::highlightedTextColourId, Colours::black);
    setColour (ComboBox::textColourId, col::gold);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float sa, float ea, Slider& s)
{
    const auto accent = s.findColour (Slider::rotarySliderFillColourId);
    const auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size = jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float arcR = size * 0.5f - size * 0.06f, arcW = jmax (2.5f, size * 0.035f), bodyR = arcR - size * 0.10f;
    const float angle = sa + pos * (ea - sa);

    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, sa, ea, true);
    g.setColour (Colour (0xff050608));
    g.strokePath (track, PathStrokeType (arcW + 2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colour (0xff272a33));
    g.strokePath (track, PathStrokeType (arcW, PathStrokeType::curved, PathStrokeType::rounded));

    float from = sa;
    if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0)
        from = sa + (float) s.valueToProportionOfLength (0.0) * (ea - sa);
    if (std::abs (angle - from) > 0.01f)
    {
        Path v;
        v.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, jmin (from, angle), jmax (from, angle), true);
        g.setColour (accent.withAlpha (0.16f));
        g.strokePath (v, PathStrokeType (arcW * 3.4f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (v, PathStrokeType (arcW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    Path body;
    body.addEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
    DropShadow (Colours::black.withAlpha (0.8f), (int) (size * 0.12f), { 0, (int) (size * 0.04f) }).drawForPath (g, body);
    g.setGradientFill (ColourGradient (Colour (0xff474b55), c.x - bodyR * 0.6f, c.y - bodyR * 0.8f, Colour (0xff0d0e12), c.x + bodyR * 0.5f, c.y + bodyR, true));
    g.fillPath (body);
    for (int i = 0; i < 40; ++i)
    {
        const float a = (float) i / 40.0f * MathConstants<float>::twoPi;
        g.setColour (i % 2 == 0 ? Colours::black.withAlpha (0.45f) : Colours::white.withAlpha (0.05f));
        g.drawLine ({ c.getPointOnCircumference (bodyR * 0.84f, a), c.getPointOnCircumference (bodyR * 0.98f, a) }, 1.0f);
    }
    g.setGradientFill (ColourGradient (Colour (0xffb9bec9), c.x, c.y - bodyR, Colour (0xff2c2f37), c.x, c.y + bodyR, false));
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2, jmax (1.0f, size * 0.012f));
    const float capR = bodyR * 0.68f;
    g.setGradientFill (ColourGradient (Colour (0xff33363f), c.x, c.y - capR, Colour (0xff101115), c.x, c.y + capR, false));
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.13f), c.x, c.y - capR, Colours::transparentWhite, c.x, c.y, false));
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2);

    Path ptr;
    ptr.startNewSubPath (c.getPointOnCircumference (bodyR * 0.30f, angle));
    ptr.lineTo (c.getPointOnCircumference (bodyR * 0.90f, angle));
    g.setColour (accent.withAlpha (0.3f));
    g.strokePath (ptr, PathStrokeType (size * 0.055f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (ptr, PathStrokeType (jmax (2.0f, size * 0.024f), PathStrokeType::curved, PathStrokeType::rounded));
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hi, bool down)
{
    const auto accent = b.findColour (TextButton::buttonOnColourId);
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool L = ! b.isConnectedOnLeft(), R = ! b.isConnectedOnRight();
    Path p;
    p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), 5.0f, 5.0f, L, R, L, R);
    if (b.getToggleState())
    {
        g.setGradientFill (ColourGradient (accent.brighter (0.35f), 0, r.getY(), accent.darker (0.25f), 0, r.getBottom(), false));
        g.fillPath (p);
        g.setColour (Colours::white.withAlpha (0.3f));
        g.drawLine (r.getX() + 3, r.getY() + 1.5f, r.getRight() - 3, r.getY() + 1.5f, 1.0f);
        g.setColour (accent.darker (0.7f));
        g.strokePath (p, PathStrokeType (1.0f));
    }
    else
    {
        g.setGradientFill (ColourGradient (Colour (down ? 0xff0c0d10 : 0xff23262d), 0, r.getY(), Colour (0xff14161a), 0, r.getBottom(), false));
        g.fillPath (p);
        g.setColour (hi ? accent.withAlpha (0.6f) : col::edge.brighter (0.2f));
        g.strokePath (p, PathStrokeType (1.0f));
    }
}

void LookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool hi, bool)
{
    g.setFont (uiFont (jmin (14.0f, b.getHeight() * 0.5f), true, 0.06f));
    g.setColour (b.getToggleState() ? Colour (0xff0b0c0f) : (hi ? col::text : col::textDim));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3, 0), Justification::centred, 1, 0.8f);
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setGradientFill (ColourGradient (Colour (0xff08090b), 0, 0, Colour (0xff181a20), 0, (float) h, false));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.isMouseOver (true) ? col::gold.withAlpha (0.6f) : col::edge.brighter (0.25f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    Path a;
    const float ax = (float) w - 18.0f, ay = (float) h * 0.5f;
    a.addTriangle (ax - 5, ay - 2.5f, ax + 5, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (col::gold);
    g.fillPath (a);
}
Font LookAndFeel::getComboBoxFont (ComboBox&) { return uiFont (15.0f, true); }
Font LookAndFeel::getPopupMenuFont()          { return uiFont (15.0f); }
void LookAndFeel::positionComboBoxText (ComboBox& b, Label& l) { l.setBounds (12, 1, b.getWidth() - 36, b.getHeight() - 2); l.setFont (getComboBoxFont (b)); }

//==============================================================================
Knob::Knob (APVTS& s, const String& id, const String& t, Colour accent, bool b) : title (t), big (b)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.23f, MathConstants<float>::pi * 2.77f, true);
    slider.setMouseDragSensitivity (big ? 260 : 200);
    slider.setColour (Slider::rotarySliderFillColourId, accent);
    addAndMakeVisible (slider);
    att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
    if (auto* p = s.getParameter (id))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
}
void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (big ? 22 : 20);
    r.removeFromBottom (big ? 30 : 26);
    const int s = jmin (r.getWidth(), r.getHeight());
    slider.setBounds (r.withSizeKeepingCentre (s, s));
}
void Knob::paint (Graphics& g)
{
    auto r = getLocalBounds();
    g.setColour (col::text);
    g.setFont (uiFont (big ? 14.0f : 12.5f, true, 0.08f));
    g.drawText (title, r.removeFromTop (big ? 22 : 20), Justification::centred, false);
    auto pill = r.removeFromBottom (big ? 30 : 26).withSizeKeepingCentre (jmin (getWidth() - 6, big ? 92 : 80), big ? 24 : 21).toFloat();
    g.setColour (col::screen);
    g.fillRoundedRectangle (pill, 4.0f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (pill, 4.0f, 1.0f);
    g.setColour (slider.findColour (Slider::rotarySliderFillColourId).brighter (0.2f));
    g.setFont (uiFont (big ? 15.0f : 13.5f, true));
    g.drawText (slider.getTextFromValue (slider.getValue()), pill, Justification::centred, false);
}

//==============================================================================
ChoiceButtons::ChoiceButtons (RangedAudioParameter& param, const StringArray& labels, Colour accent, int columns) : cols (columns)
{
    const int perRow = cols > 0 ? cols : labels.size();
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new TextButton (labels[i]));
        b->setColour (TextButton::buttonOnColourId, accent);
        const int c = i % perRow;
        int edges = 0;
        if (c > 0) edges |= Button::ConnectedOnLeft;
        if (c < perRow - 1 && i < labels.size() - 1) edges |= Button::ConnectedOnRight;
        b->setConnectedEdges (edges);
        b->onClick = [this, i] { if (att) att->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    att = std::make_unique<ParameterAttachment> (param, [this] (float v) { update (roundToInt (v)); });
    att->sendInitialUpdate();
}
void ChoiceButtons::update (int index)
{
    selected = index;
    for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState (i == index, dontSendNotification);
    if (onChange) onChange (index);
}
void ChoiceButtons::resized()
{
    const int n = buttons.size(), perRow = cols > 0 ? cols : n, rows = (n + perRow - 1) / perRow;
    const float w = (float) getWidth() / (float) perRow, gap = 6.0f;
    const float h = ((float) getHeight() - gap * (float) (rows - 1)) / (float) rows;
    for (int i = 0; i < n; ++i)
    {
        const int c = i % perRow, r = i / perRow;
        const int x0 = roundToInt ((float) c * w), x1 = roundToInt ((float) (c + 1) * w);
        buttons[i]->setBounds (x0, roundToInt ((float) r * (h + gap)), x1 - x0, roundToInt (h));
    }
}

Toggle::Toggle (APVTS& s, const String& id, const String& text, Colour accent) : TextButton (text)
{
    setClickingTogglesState (true);
    setColour (TextButton::buttonOnColourId, accent);
    att = std::make_unique<APVTS::ButtonAttachment> (s, id, *this);
}

//==============================================================================
Page::Page (VocalStripProcessor& p, int index, const String& t, const String& d, const String& onId)
    : proc (p), accent (moduleColour (index)), title (t), description (d)
{
    if (onId.isNotEmpty())
    {
        power = std::make_unique<Toggle> (p.apvts, onId, "ON", accent);
        addAndMakeVisible (*power);
    }
}
void Page::resized()
{
    if (power) power->setBounds (getWidth() - 16 - 70, 14, 70, 28);
    content = getLocalBounds().reduced (16).withTrimmedTop (48);
    layout (content);
}
Rectangle<int> Page::box (Rectangle<int> r, Graphics& g, const String& label) const
{
    g.setColour (col::screen);
    g.fillRoundedRectangle (r.toFloat(), 6.0f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 6.0f, 1.0f);
    if (label.isNotEmpty())
    {
        g.setColour (col::textDim);
        g.setFont (uiFont (11.5f, true, 0.14f));
        g.drawText (label, r.reduced (10, 6), Justification::topLeft, false);
    }
    return r.reduced (10, 8);
}
void Page::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (col::panelTop, 0, 0, col::panelBot, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
    g.setColour (accent);
    g.fillRoundedRectangle (16.0f, 14.0f, 5.0f, 28.0f, 2.5f);
    g.setFont (uiFont (20.0f, true, 0.12f));
    g.drawText (title, 30, 12, 300, 30, Justification::centredLeft, false);
    g.setColour (col::textDim);
    g.setFont (uiFont (14.0f));
    g.drawText (description, 30 + GlyphArrangement::getStringWidthInt (uiFont (20.0f, true, 0.12f), title) + 22, 12,
                getWidth() - 420, 30, Justification::centredLeft, true);
    paintContent (g);
}

//==============================================================================
// EQ graph
//==============================================================================
namespace
{
    constexpr float minDb = -24.0f, maxDb = 24.0f;
    bool usesGain (int t) { return t == vs::Bell || t == vs::LowShelf || t == vs::HighShelf; }
    Colour bandColour (int b)
    {
        static const Colour c[] = { Colour (0xffff6b6b), Colour (0xffffa94d), Colour (0xffffe066), Colour (0xff8ce99a),
                                    Colour (0xff63e6be), Colour (0xff4dabf7), Colour (0xff9775fa), Colour (0xfff783ac) };
        return c[b & 7];
    }
}

EqGraph::EqGraph (VocalStripProcessor& p) : proc (p) { setRepaintsOnMouseActivity (false); }

float EqGraph::band (int b, const char* what) const { return proc.value (VocalStripProcessor::eqId (b, what)); }
void EqGraph::set (int b, const char* what, float v)
{
    if (auto* prm = proc.apvts.getParameter (VocalStripProcessor::eqId (b, what)))
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

float EqGraph::freqToX (float f) const { return (float) getWidth() * std::log (f / 20.0f) / std::log (1000.0f); }
float EqGraph::xToFreq (float x) const { return 20.0f * std::pow (1000.0f, jlimit (0.0f, 1.0f, x / (float) getWidth())); }
float EqGraph::gainToY (float db) const { return jmap (db, maxDb, minDb, 8.0f, (float) getHeight() - 8.0f); }
float EqGraph::yToGain (float y) const { return jlimit (minDb, maxDb, jmap (y, 8.0f, (float) getHeight() - 8.0f, maxDb, minDb)); }

Point<float> EqGraph::nodePos (int b) const
{
    const int t = (int) band (b, "type");
    return { freqToX (band (b, "freq")), gainToY (usesGain (t) ? band (b, "gain") : 0.0f) };
}

int EqGraph::hitNode (Point<float> p) const
{
    int best = -1; float bestD = 14.0f;
    for (int b = 0; b < 8; ++b)
    {
        const float d = nodePos (b).getDistanceFrom (p);
        if (d < bestD) { bestD = d; best = b; }
    }
    return best;
}

void EqGraph::mouseDown (const MouseEvent& e)
{
    dragging = hitNode (e.position);
    if (dragging >= 0)
    {
        selected = dragging;
        if (onBandSelected) onBandSelected (selected);
        for (auto* w : { "freq", "gain", "on" })
            if (auto* prm = proc.apvts.getParameter (VocalStripProcessor::eqId (dragging, w))) prm->beginChangeGesture();
        if (band (dragging, "on") < 0.5f) set (dragging, "on", 1.0f);
    }
}
void EqGraph::mouseDrag (const MouseEvent& e)
{
    if (dragging < 0) return;
    set (dragging, "freq", jlimit (20.0f, 20000.0f, xToFreq (e.position.x)));
    if (usesGain ((int) band (dragging, "type")))
        set (dragging, "gain", std::round (yToGain (e.position.y) * 10.0f) / 10.0f);
}
void EqGraph::mouseUp (const MouseEvent&)
{
    if (dragging >= 0)
        for (auto* w : { "freq", "gain", "on" })
            if (auto* prm = proc.apvts.getParameter (VocalStripProcessor::eqId (dragging, w))) prm->endChangeGesture();
    dragging = -1;
}
void EqGraph::mouseDoubleClick (const MouseEvent& e)
{
    const int b = hitNode (e.position);
    if (b < 0) return;
    if (auto* prm = proc.apvts.getParameter (VocalStripProcessor::eqId (b, "on")))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (band (b, "on") > 0.5f ? 0.0f : 1.0f);
        prm->endChangeGesture();
    }
}
void EqGraph::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    int b = hitNode (e.position);
    if (b < 0) b = selected;
    if (auto* prm = proc.apvts.getParameter (VocalStripProcessor::eqId (b, "q")))
    {
        const float q = band (b, "q") * (w.deltaY > 0 ? 1.12f : 1.0f / 1.12f);
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (jlimit (0.1f, 18.0f, q)));
        prm->endChangeGesture();
    }
}

void EqGraph::tick()
{
    // pull analyzer samples
    auto& fifo = proc.analyzerFifo;
    int s1, n1, s2, n2;
    fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
    auto push = [this] (const float* d, int n) { for (int i = 0; i < n; ++i) { ring[(size_t) ringPos] = d[i]; ringPos = (ringPos + 1) & 2047; } };
    push (proc.analyzerData.data() + s1, n1);
    push (proc.analyzerData.data() + s2, n2);
    fifo.finishedRead (n1 + n2);

    std::fill (fftData.begin(), fftData.end(), 0.0f);
    for (int i = 0; i < 2048; ++i)
    {
        const float w = 0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * (float) i / 2047.0f);
        fftData[(size_t) i] = ring[(size_t) ((ringPos + i) & 2047)] * w;
    }
    fft.performFrequencyOnlyForwardTransform (fftData.data());
    for (int i = 0; i < 1024; ++i)
    {
        const float db = Decibels::gainToDecibels (fftData[(size_t) i] / 512.0f, -100.0f);
        spectrum[(size_t) i] = db > spectrum[(size_t) i] ? db : spectrum[(size_t) i] - 1.6f;
    }

    // rebuild the curve when parameters change
    String sig;
    for (int b = 0; b < 8; ++b)
        for (auto* w : { "on", "type", "freq", "gain", "q" }) sig << String (band (b, w), 3) << ",";
    sig << String (proc.value ("eq_on"));
    if (sig != curveSig)
    {
        curveSig = sig;
        curve.clear();
        const double fs = 48000.0;
        struct BC { vs::Coeffs c1, c2; int n; };
        std::vector<BC> bcs;
        if (proc.value ("eq_on") > 0.5f)
            for (int b = 0; b < 8; ++b)
                if (band (b, "on") > 0.5f)
                {
                    vs::EqBandParams p { true, (int) band (b, "type"), band (b, "freq"), band (b, "gain"), band (b, "q") };
                    std::array<float, 6> a1 {}, a2 {};
                    BC bc; bc.n = vs::eqCoefficients (p, fs, a1, a2);
                    bc.c1 = a1; if (bc.n > 1) bc.c2 = a2;
                    bcs.push_back (bc);
                }
        for (int x = 0; x <= getWidth(); x += 2)
        {
            const double f = xToFreq ((float) x);
            double m = 1.0;
            for (auto& bc : bcs)
            {
                m *= bc.c1.getMagnitudeForFrequency (f, fs);
                if (bc.n > 1) m *= bc.c2.getMagnitudeForFrequency (f, fs);
            }
            const float y = gainToY (jlimit (minDb - 6.0f, maxDb, (float) Decibels::gainToDecibels (m, -60.0)));
            if (x == 0) curve.startNewSubPath ((float) x, y); else curve.lineTo ((float) x, y);
        }
    }
    repaint();
}

void EqGraph::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::screen);
    g.fillRoundedRectangle (r, 8.0f);

    // grid
    g.setFont (uiFont (11.5f, true));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = freqToX (f);
        g.setColour (Colour (0xff1c1f26));
        g.drawVerticalLine (roundToInt (x), 4.0f, r.getBottom() - 4.0f);
        g.setColour (col::textDim.withAlpha (0.8f));
        g.drawText (f >= 1000 ? String ((int) (f / 1000)) + "k" : String ((int) f), Rectangle<float> (x + 4, r.getBottom() - 20, 40, 14), Justification::left, false);
    }
    for (float db : { -18.f, -12.f, -6.f, 0.f, 6.f, 12.f, 18.f })
    {
        const float y = gainToY (db);
        g.setColour (db == 0.0f ? Colour (0xff343844) : Colour (0xff1c1f26));
        g.drawHorizontalLine (roundToInt (y), 4.0f, r.getRight() - 4.0f);
        g.setColour (col::textDim.withAlpha (0.8f));
        g.drawText ((db > 0 ? "+" : "") + String ((int) db), Rectangle<float> (r.getRight() - 36, y - 14, 30, 14), Justification::right, false);
    }

    // spectrum (mapped -90..0 dBFS onto the graph height)
    {
        Path sp;
        const double fs = proc.getSampleRateSafe();
        sp.startNewSubPath (0, r.getBottom());
        for (int x = 0; x <= getWidth(); x += 2)
        {
            const float f = xToFreq ((float) x);
            const float fb = jlimit (1.0f, 1022.0f, f / (float) fs * 2048.0f);
            const int bin = (int) fb;
            const float db = spectrum[(size_t) bin] + (fb - (float) bin) * (spectrum[(size_t) bin + 1] - spectrum[(size_t) bin]);
            const float y = jmap (jlimit (-90.0f, 0.0f, db), -90.0f, 0.0f, r.getBottom(), r.getY() + r.getHeight() * 0.1f);
            sp.lineTo ((float) x, y);
        }
        sp.lineTo (r.getRight(), r.getBottom());
        sp.closeSubPath();
        g.setGradientFill (ColourGradient (Colour (0xff4fd8ff).withAlpha (0.22f), 0, r.getY(), Colour (0xff4fd8ff).withAlpha (0.03f), 0, r.getBottom(), false));
        g.fillPath (sp);
    }

    // curve
    {
        Path fill (curve);
        fill.lineTo (r.getRight(), gainToY (0)); fill.lineTo (0, gainToY (0)); fill.closeSubPath();
        g.setColour (Colour (0xff4fd8ff).withAlpha (0.12f));
        g.fillPath (fill);
        g.setColour (Colour (0xff4fd8ff).withAlpha (0.35f));
        g.strokePath (curve, PathStrokeType (6.0f));
        g.setColour (Colour (0xffc9f3ff));
        g.strokePath (curve, PathStrokeType (2.0f));
    }

    // nodes
    for (int b = 0; b < 8; ++b)
    {
        const auto p = nodePos (b);
        const bool on = band (b, "on") > 0.5f;
        const auto c = bandColour (b);
        const float rad = b == selected ? 11.0f : 9.0f;
        if (b == selected)
        {
            g.setColour (c.withAlpha (0.25f));
            g.fillEllipse (p.x - 18, p.y - 18, 36, 36);
        }
        g.setColour (on ? c : Colour (0xff3a3e48));
        g.fillEllipse (p.x - rad, p.y - rad, rad * 2, rad * 2);
        g.setColour (Colours::black.withAlpha (0.8f));
        g.drawEllipse (p.x - rad, p.y - rad, rad * 2, rad * 2, 1.5f);
        g.setColour (on ? Colours::black : col::textDim);
        g.setFont (uiFont (12.0f, true));
        g.drawText (String (b + 1), Rectangle<float> (p.x - rad, p.y - rad, rad * 2, rad * 2), Justification::centred, false);
    }

    g.setColour (col::textDim);
    g.setFont (uiFont (12.0f));
    g.drawText ("Drag a point to move it   |   Mouse wheel = width (Q)   |   Double-click = on / off", r.reduced (12, 8), Justification::topLeft, false);
}

//==============================================================================
// Pages
//==============================================================================
namespace
{
    void placeRow (std::initializer_list<Component*> items, Rectangle<int> area)
    {
        const int n = (int) items.size();
        if (n == 0) return;
        const int w = area.getWidth() / n;
        int i = 0;
        for (auto* c : items) c->setBounds (area.getX() + i++ * w, area.getY(), w, area.getHeight());
    }

    float vuToFrac (float db, float range) { return jlimit (0.0f, 1.0f, db / range); }

    void drawGrBar (Graphics& g, Rectangle<float> r, float grDb, float range, Colour accent, const String& label)
    {
        g.setColour (col::textDim);
        g.setFont (uiFont (12.0f, true, 0.1f));
        g.drawText (label, r.removeFromTop (18), Justification::centredLeft, false);
        g.setColour (col::text);
        g.setFont (uiFont (22.0f, true));
        g.drawText ("-" + String (grDb, 1) + " dB", r.removeFromTop (30), Justification::centredLeft, false);
        auto bar = r.removeFromTop (14);
        g.setColour (Colour (0xff15171c));
        g.fillRoundedRectangle (bar, 4.0f);
        const float f = vuToFrac (grDb, range);
        if (f > 0.0f)
        {
            auto fr = bar.withLeft (bar.getRight() - bar.getWidth() * f);
            g.setGradientFill (ColourGradient (accent.darker (0.3f), fr.getX(), 0, accent.brighter (0.3f), bar.getRight(), 0, false));
            g.fillRoundedRectangle (fr, 4.0f);
        }
        g.setColour (col::textDim.withAlpha (0.7f));
        g.setFont (uiFont (10.5f));
        auto scale = r.removeFromTop (14);
        for (int k = 0; k <= 4; ++k)
        {
            const float v = range * (float) k / 4.0f;
            const float x = bar.getRight() - bar.getWidth() * (float) k / 4.0f;
            g.drawText (String ((int) v), Rectangle<float> (x - 15, scale.getY(), 30, 14), Justification::centred, false);
        }
    }

    // ------------------------------------------------------------------ EQ
    class EqPage : public Page
    {
    public:
        explicit EqPage (VocalStripProcessor& p)
            : Page (p, 0, "EQUALIZER", "8 bands - find and cut resonances, shape the tone", "eq_on"), graph (p)
        {
            addAndMakeVisible (graph);
            for (int b = 0; b < 8; ++b)
            {
                auto* sel = bandSelect.add (new TextButton (String (b + 1)));
                sel->setColour (TextButton::buttonOnColourId, bandColour (b));
                sel->setConnectedEdges ((b > 0 ? Button::ConnectedOnLeft : 0) | (b < 7 ? Button::ConnectedOnRight : 0));
                sel->onClick = [this, b] { showBand (b); };
                addAndMakeVisible (sel);

                auto& s = sets.emplace_back();
                s.on = std::make_unique<Toggle> (p.apvts, VocalStripProcessor::eqId (b, "on"), "BAND ON", bandColour (b));
                s.type = std::make_unique<ChoiceButtons> (*p.apvts.getParameter (VocalStripProcessor::eqId (b, "type")),
                                                          StringArray { "BELL", "LO SHELF", "HI SHELF", "NOTCH", "LOW CUT", "HIGH CUT", "BAND" }, bandColour (b), 4);
                s.freq = std::make_unique<Knob> (p.apvts, VocalStripProcessor::eqId (b, "freq"), "FREQUENCY", bandColour (b), true);
                s.gain = std::make_unique<Knob> (p.apvts, VocalStripProcessor::eqId (b, "gain"), "GAIN", bandColour (b), true);
                s.q    = std::make_unique<Knob> (p.apvts, VocalStripProcessor::eqId (b, "q"), "Q / WIDTH", bandColour (b), true);
                for (Component* c : std::initializer_list<Component*> { s.on.get(), s.type.get(), s.freq.get(), s.gain.get(), s.q.get() })
                    addChildComponent (c);
            }
            graph.onBandSelected = [this] (int b) { showBand (b); };
            showBand (0);
        }
        void layout (Rectangle<int> a) override
        {
            graph.setBounds (a.removeFromTop (330));
            a.removeFromTop (12);
            auto left = a.removeFromLeft (560);
            auto row = left.removeFromTop (32);
            const int bw = row.getWidth() / 8;
            for (int b = 0; b < 8; ++b) bandSelect[b]->setBounds (row.getX() + b * bw, row.getY(), bw, 32);
            left.removeFromTop (10);
            for (auto& s : sets)
            {
                auto l = left;
                s.on->setBounds (l.removeFromLeft (100).removeFromTop (70));
                l.removeFromLeft (10);
                s.type->setBounds (l.removeFromTop (70));
                placeRow ({ s.freq.get(), s.gain.get(), s.q.get() }, a.withTrimmedLeft (20));
            }
        }
        void showBand (int b)
        {
            graph.selected = b;
            for (int i = 0; i < 8; ++i)
            {
                bandSelect[i]->setToggleState (i == b, dontSendNotification);
                auto& s = sets[(size_t) i];
                for (Component* c : std::initializer_list<Component*> { s.on.get(), s.type.get(), s.freq.get(), s.gain.get(), s.q.get() })
                    c->setVisible (i == b);
            }
            graph.repaint();
        }
        void tick() override { graph.tick(); }
    private:
        struct BandSet { std::unique_ptr<Toggle> on; std::unique_ptr<ChoiceButtons> type; std::unique_ptr<Knob> freq, gain, q; };
        EqGraph graph;
        OwnedArray<TextButton> bandSelect;
        std::vector<BandSet> sets;
    };

    // ------------------------------------------------------------ COMPRESSOR
    class CompPage : public Page
    {
    public:
        explicit CompPage (VocalStripProcessor& p)
            : Page (p, 1, "COMPRESSOR", "Choose the character: smooth Opto, clean VCA or fast punchy FET", "comp_on"),
              model (*p.apvts.getParameter ("comp_model"), { "OPTO", "VCA", "FET" }, accent),
              thr (p.apvts, "comp_thresh", "THRESHOLD", accent, true), ratio (p.apvts, "comp_ratio", "RATIO", accent, true),
              atk (p.apvts, "comp_attack", "ATTACK", accent, true), rel (p.apvts, "comp_release", "RELEASE", accent, true),
              mk (p.apvts, "comp_makeup", "MAKEUP", accent, true), mix (p.apvts, "comp_mix", "MIX", accent, true)
        {
            for (Component* c : std::initializer_list<Component*> { &model, &thr, &ratio, &atk, &rel, &mk, &mix }) addAndMakeVisible (c);
            model.onChange = [this] (int) { repaint(); };
        }
        void layout (Rectangle<int> a) override
        {
            display = a.removeFromRight (300);
            a.removeFromRight (16);
            model.setBounds (a.removeFromTop (54));
            a.removeFromTop (10);
            descArea = a.removeFromTop (30);
            a.removeFromTop (6);
            auto r1 = a.removeFromTop (a.getHeight() / 2);
            placeRow ({ &thr, &ratio, &atk }, r1);
            placeRow ({ &rel, &mk, &mix }, a);
        }
        void paintContent (Graphics& g) override
        {
            static const char* d[] = { "OPTO - slow, smooth, program-dependent (LA-2A style). Warm tube colour.",
                                       "VCA - clean and precise. Does exactly what the knobs say.",
                                       "FET - very fast and aggressive (1176 style). Adds bite and excitement." };
            g.setColour (accent.brighter (0.3f));
            g.setFont (uiFont (14.0f));
            g.drawText (d[jlimit (0, 2, model.getSelected())], descArea, Justification::centredLeft, true);

            auto r = box (display, g, "TRANSFER CURVE").toFloat();
            auto graphR = r.withTrimmedTop (20).removeFromTop (r.getWidth() - 20);
            const float thrDb = proc.value ("comp_thresh"), rt = proc.value ("comp_ratio");
            const float knee = model.getSelected() == 0 ? 10.0f : model.getSelected() == 2 ? 2.0f : 4.0f;
            auto tx = [&] (float db) { return graphR.getX() + (db + 60.0f) / 60.0f * graphR.getWidth(); };
            auto ty = [&] (float db) { return graphR.getBottom() - (db + 60.0f) / 60.0f * graphR.getHeight(); };
            g.setColour (Colour (0xff1c1f26));
            for (float db = -48; db < 0; db += 12) { g.drawVerticalLine (roundToInt (tx (db)), graphR.getY(), graphR.getBottom()); g.drawHorizontalLine (roundToInt (ty (db)), graphR.getX(), graphR.getRight()); }
            g.setColour (col::edge.brighter (0.3f));
            g.drawLine (tx (-60), ty (-60), tx (0), ty (0), 1.0f);
            Path c;
            for (int i = 0; i <= 60; ++i)
            {
                const float in = -60.0f + (float) i;
                const float out = in + vs::kneeGain (in, thrDb, rt, knee);
                if (i == 0) c.startNewSubPath (tx (in), ty (out)); else c.lineTo (tx (in), ty (out));
            }
            g.setColour (accent);
            g.strokePath (c, PathStrokeType (2.5f));
            r.removeFromTop (graphR.getHeight() + 30);
            drawGrBar (g, r, proc.compGr.load(), 20.0f, accent, "GAIN REDUCTION");
        }
        void tick() override { repaint (display); }
    private:
        ChoiceButtons model;
        Knob thr, ratio, atk, rel, mk, mix;
        Rectangle<int> display, descArea;
    };

    // ------------------------------------------------------------ DE-ESSER
    class DeEssPage : public Page
    {
    public:
        explicit DeEssPage (VocalStripProcessor& p)
            : Page (p, 2, "DE-ESSER", "Tames harsh S / SH / CH sounds (sibilance)", "ds_on"),
              freq (p.apvts, "ds_freq", "FREQUENCY", accent, true), thr (p.apvts, "ds_thresh", "THRESHOLD", accent, true),
              range (p.apvts, "ds_range", "RANGE", accent, true),
              mode (*p.apvts.getParameter ("ds_mode"), { "SPLIT", "WIDE" }, accent),
              listen (p.apvts, "ds_listen", "LISTEN (solo the S band)", accent)
        {
            for (Component* c : std::initializer_list<Component*> { &freq, &thr, &range, &mode, &listen }) addAndMakeVisible (c);
        }
        void layout (Rectangle<int> a) override
        {
            display = a.removeFromRight (380);
            a.removeFromRight (16);
            auto top = a.removeFromTop (44);
            mode.setBounds (top.removeFromLeft (220));
            top.removeFromLeft (16);
            listen.setBounds (top.removeFromLeft (260));
            a.removeFromTop (20);
            placeRow ({ &freq, &thr, &range }, a.removeFromTop (220));
            help = a;
        }
        void paintContent (Graphics& g) override
        {
            g.setColour (col::textDim);
            g.setFont (uiFont (13.5f));
            g.drawFittedText ("Tip: press LISTEN and sweep FREQUENCY until you hear mostly the 'sss'. Then turn LISTEN off and lower THRESHOLD until the S sounds are smooth.\n"
                              "SPLIT reduces only the high S band (most natural). WIDE turns the whole voice down on each S.",
                              help.reduced (4), Justification::topLeft, 4);
            auto r = box (display, g, "SIBILANCE").toFloat();
            auto fr = r.withTrimmedTop (22).removeFromTop (150);
            auto fx = [&] (float f) { return fr.getX() + std::log (f / 1000.0f) / std::log (16.0f) * fr.getWidth(); };
            const float f = proc.value ("ds_freq");
            g.setColour (Colour (0xff1c1f26));
            for (float k : { 2000.f, 4000.f, 8000.f }) g.drawVerticalLine (roundToInt (fx (k)), fr.getY(), fr.getBottom());
            Path bump;
            for (int i = 0; i <= 100; ++i)
            {
                const float ff = 1000.0f * std::pow (16.0f, (float) i / 100.0f);
                const float d = std::log2 (ff / f);
                const float h = std::exp (-d * d * 3.0f);
                const float y = fr.getBottom() - h * fr.getHeight() * 0.85f;
                if (i == 0) bump.startNewSubPath (fx (ff), y); else bump.lineTo (fx (ff), y);
            }
            g.setColour (accent);
            g.strokePath (bump, PathStrokeType (2.5f));
            g.setColour (col::textDim);
            g.setFont (uiFont (11.5f));
            for (float k : { 2000.f, 4000.f, 8000.f }) g.drawText (String ((int) (k / 1000)) + "k", Rectangle<float> (fx (k) + 3, fr.getBottom() - 14, 30, 14), Justification::left, false);
            r.removeFromTop (190);
            drawGrBar (g, r, proc.deessGr.load(), 24.0f, accent, "S REDUCTION");
        }
        void tick() override { repaint (display); }
    private:
        Knob freq, thr, range;
        ChoiceButtons mode;
        Toggle listen;
        Rectangle<int> display, help;
    };

    // ------------------------------------------------------------ SATURATION
    class SatPage : public Page
    {
    public:
        explicit SatPage (VocalStripProcessor& p)
            : Page (p, 3, "SATURATION", "3-band harmonic colour + Air and Mid-tone exciters", "sat_on"),
              type (*p.apvts.getParameter ("sat_type"), { "TUBE", "TAPE", "WARM" }, accent),
              low (p.apvts, "sat_low", "LOW DRIVE", accent, true), mid (p.apvts, "sat_mid", "MID DRIVE", accent, true),
              high (p.apvts, "sat_high", "HIGH DRIVE", accent, true),
              xl (p.apvts, "sat_xlow", "LOW / MID", accent), xh (p.apvts, "sat_xhigh", "MID / HIGH", accent),
              tone (p.apvts, "sat_tone", "MID TONE", Colour (0xffffd166), true), air (p.apvts, "sat_air", "AIR", Colour (0xff9be7ff), true),
              out (p.apvts, "sat_out", "OUTPUT", accent)
        {
            for (Component* c : std::initializer_list<Component*> { &type, &low, &mid, &high, &xl, &xh, &tone, &air, &out }) addAndMakeVisible (c);
        }
        void layout (Rectangle<int> a) override
        {
            auto top = a.removeFromTop (44);
            type.setBounds (top.removeFromLeft (330));
            a.removeFromTop (10);
            display = a.removeFromTop (86);
            a.removeFromTop (8);
            placeRow ({ &low, &mid, &high, &tone, &air }, a.removeFromTop (a.getHeight() / 2 + 20));
            placeRow ({ &xl, &xh, &out }, a.withSizeKeepingCentre (a.getWidth() * 3 / 5, a.getHeight()));
        }
        void paintContent (Graphics& g) override
        {
            auto r = box (display, g, "BANDS").toFloat().withTrimmedTop (14);
            auto fx = [&] (float f) { return r.getX() + std::log (f / 20.0f) / std::log (1000.0f) * r.getWidth(); };
            const float x1 = fx (proc.value ("sat_xlow")), x2 = fx (proc.value ("sat_xhigh"));
            const float d[3] = { proc.value ("sat_low"), proc.value ("sat_mid"), proc.value ("sat_high") };
            const float xs[4] = { r.getX(), x1, x2, r.getRight() };
            const char* names[3] = { "LOW", "MID", "HIGH" };
            for (int b = 0; b < 3; ++b)
            {
                auto br = Rectangle<float> (xs[b] + 2, r.getY(), xs[b + 1] - xs[b] - 4, r.getHeight());
                const float h = br.getHeight() * (0.08f + 0.92f * d[b] * 0.01f);
                auto fill = br.withTop (br.getBottom() - h);
                g.setGradientFill (ColourGradient (accent.withAlpha (0.85f), 0, fill.getY(), accent.withAlpha (0.15f), 0, br.getBottom(), false));
                g.fillRoundedRectangle (fill, 4.0f);
                g.setColour (col::text);
                g.setFont (uiFont (13.0f, true, 0.1f));
                g.drawText (String (names[b]) + "  " + String (roundToInt (d[b])) + "%", br.reduced (8, 4), Justification::topLeft, false);
            }
        }
        void tick() override { repaint (display); }
    private:
        ChoiceButtons type;
        Knob low, mid, high, xl, xh, tone, air, out;
        Rectangle<int> display;
    };

    // ------------------------------------------------------------ GLUE
    class GluePage : public Page
    {
    public:
        explicit GluePage (VocalStripProcessor& p)
            : Page (p, 4, "GLUE COMPRESSOR", "Bus-style glue that holds the whole vocal together", "glue_on"),
              thr (p.apvts, "glue_thresh", "THRESHOLD", accent, true), mk (p.apvts, "glue_makeup", "MAKEUP", accent, true),
              mix (p.apvts, "glue_mix", "MIX", accent, true),
              ratio (*p.apvts.getParameter ("glue_ratio"), vs::Glue::ratioNames(), accent),
              atk (*p.apvts.getParameter ("glue_attack"), vs::Glue::attackNames(), accent),
              rel (*p.apvts.getParameter ("glue_release"), vs::Glue::releaseNames(), accent)
        {
            for (Component* c : std::initializer_list<Component*> { &thr, &mk, &mix, &ratio, &atk, &rel }) addAndMakeVisible (c);
        }
        void layout (Rectangle<int> a) override
        {
            display = a.removeFromRight (330);
            a.removeFromRight (16);
            auto rows = a.removeFromTop (210);
            const int rh = 70;
            ratioL = rows.removeFromTop (rh); ratio.setBounds (ratioL.withTrimmedLeft (110).withSizeKeepingCentre (ratioL.getWidth() - 110, 40));
            atkL = rows.removeFromTop (rh);   atk.setBounds (atkL.withTrimmedLeft (110).withSizeKeepingCentre (atkL.getWidth() - 110, 40));
            relL = rows.removeFromTop (rh);   rel.setBounds (relL.withTrimmedLeft (110).withSizeKeepingCentre (relL.getWidth() - 110, 40));
            a.removeFromTop (10);
            placeRow ({ &thr, &mk, &mix }, a);
        }
        void paintContent (Graphics& g) override
        {
            g.setColour (col::text);
            g.setFont (uiFont (14.0f, true, 0.1f));
            g.drawText ("RATIO", ratioL.withWidth (100), Justification::centredLeft, false);
            g.drawText ("ATTACK ms", atkL.withWidth (100), Justification::centredLeft, false);
            g.drawText ("RELEASE s", relL.withWidth (100), Justification::centredLeft, false);

            // analog-style needle meter
            auto r = box (display, g, "GLUE GR").toFloat();
            auto m = r.withTrimmedTop (20).removeFromTop (200);
            const Point<float> pivot { m.getCentreX(), m.getBottom() + 20 };
            const float R = m.getHeight() * 0.9f;
            g.setColour (Colour (0xfff3e6c8));
            g.fillRoundedRectangle (m, 6.0f);
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (m.toNearestInt());
            const float span = 0.8f;
            for (int k = 0; k <= 5; ++k)
            {
                const float a = -span + 2.0f * span * (1.0f - (float) k / 5.0f);
                g.setColour (Colour (0xff2a2016));
                g.drawLine ({ pivot.getPointOnCircumference (R - 14, a), pivot.getPointOnCircumference (R, a) }, 1.6f);
                g.setFont (uiFont (13.0f, true));
                g.drawText (String (k * 4), Rectangle<float> (30, 16).withCentre (pivot.getPointOnCircumference (R - 30, a)), Justification::centred, false);
            }
            const float frac = jlimit (0.0f, 1.0f, proc.glueGr.load() / 20.0f);
            const float a = -span + 2.0f * span * (1.0f - frac);
            g.setColour (Colour (0xffc8432f));
            g.drawLine ({ pivot, pivot.getPointOnCircumference (R - 4, a) }, 2.2f);
            g.setColour (Colour (0xff2a2016));
            g.setFont (uiFont (14.0f, true, 0.1f));
            g.drawText ("GAIN REDUCTION dB", m.reduced (8), Justification::centredBottom, false);
        }
        void tick() override { repaint (display); }
    private:
        Knob thr, mk, mix;
        ChoiceButtons ratio, atk, rel;
        Rectangle<int> display, ratioL, atkL, relL;
    };

    // ------------------------------------------------------------ DELAY
    class DelayPage : public Page
    {
    public:
        explicit DelayPage (VocalStripProcessor& p)
            : Page (p, 5, "DELAY", "Mono, Stereo or Ping-Pong echoes, synced to your song", "dly_on"),
              mode (*p.apvts.getParameter ("dly_mode"), { "MONO", "STEREO", "PING-PONG" }, accent),
              sync (p.apvts, "dly_sync", "SYNC TO TEMPO", accent),
              divL (*p.apvts.getParameter ("dly_div"), VocalStripProcessor::delayDivisions(), accent, 6),
              divR (*p.apvts.getParameter ("dly_div_r"), VocalStripProcessor::delayDivisions(), rightColour(), 6),
              timeL (p.apvts, "dly_time", "TIME", accent), fbL (p.apvts, "dly_fb", "FEEDBACK", accent),
              timeR (p.apvts, "dly_time_r", "TIME", rightColour()), fbR (p.apvts, "dly_fb_r", "FEEDBACK", rightColour()),
              lc (p.apvts, "dly_lowcut", "LOW CUT", accent), hc (p.apvts, "dly_highcut", "HIGH CUT", accent),
              width (p.apvts, "dly_width", "WIDTH", accent), duck (p.apvts, "dly_duck", "DUCK", accent),
              mix (p.apvts, "dly_mix", "LEVEL", accent, true)
        {
            for (Component* c : std::initializer_list<Component*> { &mode, &sync, &divL, &divR, &timeL, &fbL, &timeR, &fbR,
                                                                    &lc, &hc, &width, &duck, &mix })
                addAndMakeVisible (c);
            mode.onChange = [this] (int) { updateVisibility(); };
        }
        static Colour rightColour() { return Colour (0xffffd166); }

        void layout (Rectangle<int> a) override
        {
            auto top = a.removeFromTop (44);
            mode.setBounds (top.removeFromLeft (420));
            top.removeFromLeft (16);
            sync.setBounds (top.removeFromLeft (200));
            a.removeFromTop (10);
            display = a.removeFromTop (96);
            a.removeFromTop (10);

            const int colW = 300;
            leftCol = a.removeFromLeft (colW);
            a.removeFromLeft (12);
            rightCol = a.removeFromLeft (colW);
            a.removeFromLeft (16);

            auto place = [] (Rectangle<int> col, ChoiceButtons& div, Knob& time, Knob& fb)
            {
                col.removeFromTop (22);                     // column title
                div.setBounds (col.removeFromTop (64));
                col.removeFromTop (6);
                placeRow ({ &time, &fb }, col);
            };
            place (leftCol, divL, timeL, fbL);
            place (rightCol, divR, timeR, fbR);

            auto r1 = a.removeFromTop (a.getHeight() / 2);
            placeRow ({ &mix, &width, &duck }, r1);
            placeRow ({ &lc, &hc }, a.withSizeKeepingCentre (a.getWidth() * 2 / 3, a.getHeight()));
            updateVisibility();
        }

        void updateVisibility()
        {
            const bool stereo = mode.getSelected() == 1;
            for (Component* c : std::initializer_list<Component*> { &divR, &timeR, &fbR }) c->setVisible (stereo);
            repaint();
        }

        void paintContent (Graphics& g) override
        {
            const bool synced = proc.value ("dly_sync") > 0.5f;
            const bool stereo = mode.getSelected() == 1;
            for (Knob* k : { &timeL, &timeR }) k->setAlpha (synced ? 0.35f : 1.0f);
            for (ChoiceButtons* d : { &divL, &divR }) d->setAlpha (synced ? 1.0f : 0.35f);

            // column titles
            auto title = [&] (Rectangle<int> col, const String& t, Colour c)
            {
                auto r = col.removeFromTop (20).toFloat();
                g.setColour (c);
                g.fillRoundedRectangle (r.removeFromLeft (4), 2.0f);
                g.setFont (uiFont (13.5f, true, 0.12f));
                g.drawText (t, r.withTrimmedLeft (8), Justification::centredLeft, false);
            };
            title (leftCol, stereo ? "LEFT" : "TIME  (both sides)", accent);
            if (stereo)
                title (rightCol, "RIGHT", rightColour());
            else
            {
                g.setColour (col::textDim.withAlpha (0.8f));
                g.setFont (uiFont (13.0f));
                g.drawFittedText ("Choose STEREO to set the left and right echoes separately "
                                  "(for example LEFT 1/8 and RIGHT 1/8 dotted for a wide stereo echo).",
                                  rightCol.reduced (12, 30), Justification::centred, 4);
            }

            // echo picture: real echo times for each side
            auto r = box (display, g, "ECHOES  (" + String (proc.hostBpm.load(), 0) + " BPM)").toFloat().withTrimmedTop (16);
            const int m = (int) proc.value ("dly_mode");
            auto msFor = [&] (const char* timeId, const char* divId)
            {
                if (! synced) return proc.value (timeId);
                static const float beats[] = { 0.125f, 1.0f / 6.0f, 0.25f, 0.375f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f, 2.0f, 4.0f };
                return (float) (60000.0 / jlimit (30.0, 300.0, proc.hostBpm.load())) * beats[jlimit (0, 11, (int) proc.value (divId))];
            };
            const float tL = msFor ("dly_time", "dly_div");
            const float tR = m == 1 ? msFor ("dly_time_r", "dly_div_r") : tL;
            const float fL = proc.value ("dly_fb") * 0.01f, fR = m == 1 ? proc.value ("dly_fb_r") * 0.01f : fL;
            const float spanMs = jmax (tL, tR) * 6.5f;
            auto tx = [&] (float ms) { return r.getX() + 16.0f + ms / spanMs * (r.getWidth() - 26.0f); };

            auto drawSide = [&] (bool top, float t, float fb, Colour c, int startIndex, int step)
            {
                float amp = 1.0f;
                for (int k = 1; k < 14; ++k)
                {
                    const float ms = t * (float) (startIndex + (k - 1) * step);
                    if (ms > spanMs || amp < 0.02f) break;
                    const float h = r.getHeight() * 0.44f * std::sqrt (amp);
                    g.setColour (c.withAlpha (0.25f + 0.75f * amp));
                    const float x = tx (ms);
                    if (top) g.fillRoundedRectangle (x, r.getCentreY() - h, 9.0f, h, 3.0f);
                    else     g.fillRoundedRectangle (x, r.getCentreY() + 2, 9.0f, h, 3.0f);
                    amp *= fb;
                }
            };
            if (m == 2)      { drawSide (true, tL, fL * fL, accent, 1, 2); drawSide (false, tL, fL * fL, accent, 2, 2); }
            else if (m == 1) { drawSide (true, tL, fL, accent, 1, 1); drawSide (false, tR, fR, rightColour(), 1, 1); }
            else             { drawSide (true, tL, fL, accent, 1, 1); drawSide (false, tL, fL, accent, 1, 1); }

            g.setColour (col::textDim);
            g.setFont (uiFont (11.5f, true));
            g.drawText ("L  " + String (roundToInt (tL)) + " ms", r.withHeight (14), Justification::topLeft, false);
            g.drawText ("R  " + String (roundToInt (tR)) + " ms", r.withTop (r.getBottom() - 14), Justification::bottomLeft, false);
        }
        void tick() override { repaint (display); }
    private:
        ChoiceButtons mode;
        Toggle sync;
        ChoiceButtons divL, divR;
        Knob timeL, fbL, timeR, fbR, lc, hc, width, duck, mix;
        Rectangle<int> display, leftCol, rightCol;
    };

    // ------------------------------------------------------------ REVERB
    class ReverbPage : public Page
    {
    public:
        explicit ReverbPage (VocalStripProcessor& p)
            : Page (p, 6, "REVERB", "Place the vocal in a space - room, plate, hall and more", "rev_on"),
              decay (p.apvts, "rev_decay", "DECAY", accent, true), pre (p.apvts, "rev_pre", "PRE-DELAY", accent, true),
              size (p.apvts, "rev_size", "SIZE", accent, true), lc (p.apvts, "rev_lowcut", "LOW CUT", accent, true),
              hc (p.apvts, "rev_highcut", "HIGH CUT", accent, true), mix (p.apvts, "rev_mix", "LEVEL", accent, true)
        {
            for (int i = 0; i < 9; ++i)
            {
                auto* b = envs.add (new TextButton (gv::envTypes()[(size_t) i].name));
                b->setColour (TextButton::buttonOnColourId, accent);
                b->onClick = [this, i] { proc.applyReverbEnvironment (i); };
                addAndMakeVisible (b);
            }
            typeAtt = std::make_unique<ParameterAttachment> (*p.apvts.getParameter ("rev_type"), [this] (float v)
            {
                for (int i = 0; i < envs.size(); ++i) envs[i]->setToggleState (i == roundToInt (v), dontSendNotification);
                repaint();
            });
            typeAtt->sendInitialUpdate();
            for (Component* c : std::initializer_list<Component*> { &decay, &pre, &size, &lc, &hc, &mix }) addAndMakeVisible (c);
        }
        void layout (Rectangle<int> a) override
        {
            auto left = a.removeFromLeft (360);
            const int bw = (left.getWidth() - 16) / 3, bh = 52;
            for (int i = 0; i < 9; ++i) envs[i]->setBounds (left.getX() + (i % 3) * (bw + 8), left.getY() + (i / 3) * (bh + 8), bw, bh);
            display = left.withTrimmedTop (3 * (bh + 8) + 8);
            a.removeFromLeft (16);
            auto r1 = a.removeFromTop (a.getHeight() / 2);
            placeRow ({ &decay, &pre, &size }, r1);
            placeRow ({ &lc, &hc, &mix }, a);
        }
        void paintContent (Graphics& g) override
        {
            auto r = box (display, g, "DECAY").toFloat().withTrimmedTop (16);
            const float dec = proc.value ("rev_decay"), pd = proc.value ("rev_pre") * 0.001f;
            const float total = jmax (0.5f, pd + dec * 1.1f);
            Path env;
            env.startNewSubPath (r.getX(), r.getBottom());
            const float x0 = r.getX() + pd / total * r.getWidth();
            env.lineTo (x0, r.getBottom()); env.lineTo (x0 + 2, r.getY() + 6);
            for (int i = 0; i <= 50; ++i)
            {
                const float t = dec * 1.1f * (float) i / 50.0f;
                const float y = jmap (jlimit (-60.0f, 0.0f, -60.0f * t / dec), -60.0f, 0.0f, r.getBottom(), r.getY() + 6);
                env.lineTo (x0 + 2 + t / total * r.getWidth(), y);
            }
            env.lineTo (r.getRight(), r.getBottom()); env.closeSubPath();
            g.setGradientFill (ColourGradient (accent.withAlpha (0.6f), 0, r.getY(), accent.withAlpha (0.05f), 0, r.getBottom(), false));
            g.fillPath (env);
            g.setColour (col::text);
            g.setFont (uiFont (14.0f, true));
            g.drawText (String (dec, 2) + " s", r.reduced (6, 2), Justification::topRight, false);
        }
    private:
        OwnedArray<TextButton> envs;
        std::unique_ptr<ParameterAttachment> typeAtt;
        Knob decay, pre, size, lc, hc, mix;
        Rectangle<int> display;
    };

    const char* tileNames[] = { "EQ", "COMP", "DE-ESS", "SATURATE", "GLUE", "DELAY", "REVERB" };
    const char* tileOnIds[] = { "eq_on", "comp_on", "ds_on", "sat_on", "glue_on", "dly_on", "rev_on" };

    const Rectangle<int> pageArea { 24, 156, 1012, 552 };
    const Rectangle<int> sideArea { 1052, 76, 124, 632 };
}

//==============================================================================
MainPanel::MainPanel (VocalStripProcessor& p)
    : proc (p), inGain (p.apvts, "in_gain", "INPUT", col::gold), outGain (p.apvts, "out_gain", "OUTPUT", col::gold)
{
    setLookAndFeel (&lnf);
    setOpaque (true);

    pages.add (new EqPage (p));
    pages.add (new CompPage (p));
    pages.add (new DeEssPage (p));
    pages.add (new SatPage (p));
    pages.add (new GluePage (p));
    pages.add (new DelayPage (p));
    pages.add (new ReverbPage (p));
    for (auto* pg : pages) addChildComponent (pg);

    for (int i = 0; i < 7; ++i)
    {
        auto* t = tilePower.add (new Toggle (p.apvts, tileOnIds[i], "", moduleColour (i)));
        t->setTooltip ("Module on / off");
        addAndMakeVisible (t);
    }
    addAndMakeVisible (inGain);
    addAndMakeVisible (outGain);
    addAndMakeVisible (presetBox);
    for (int i = 0; i < p.getNumPrograms(); ++i) presetBox.addItem (p.getProgramName (i), i + 1);
    presetBox.setSelectedId (p.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0 && id - 1 != proc.getCurrentProgram()) proc.setCurrentProgram (id - 1);
    };

    select (0);
    startTimerHz (30);
}

MainPanel::~MainPanel() { stopTimer(); setLookAndFeel (nullptr); }

Rectangle<int> MainPanel::tileBounds (int i) const
{
    const int w = (pageArea.getWidth() - 6 * 10) / 7;
    return { pageArea.getX() + i * (w + 10), 76, w, 66 };
}

void MainPanel::select (int page)
{
    current = jlimit (0, pages.size() - 1, page);
    for (int i = 0; i < pages.size(); ++i) pages[i]->setVisible (i == current);
    repaint();
}

void MainPanel::resized()
{
    for (auto* pg : pages) pg->setBounds (pageArea);
    for (int i = 0; i < 7; ++i)
    {
        auto t = tileBounds (i);
        tilePower[i]->setBounds (t.getRight() - 30, t.getY() + 10, 20, 20);
    }
    inGain.setBounds (sideArea.getX(), sideArea.getY(), sideArea.getWidth(), 150);
    outGain.setBounds (sideArea.getX(), sideArea.getY() + 156, sideArea.getWidth(), 150);
    presetBox.setBounds (W - 24 - 280, 18, 280, 32);

    texture = Image (Image::RGB, W, H, true);
    Graphics g (texture);
    g.setGradientFill (ColourGradient (col::bgTop, 0, 0, col::bgBot, 0, (float) H, false));
    g.fillAll();
    Random rnd (5);
    for (int i = 0; i < 12000; ++i)
    {
        g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (rnd.nextFloat() * 0.05f));
        g.fillRect (rnd.nextFloat() * W, rnd.nextFloat() * H, 1.0f, 1.0f);
    }
}

void MainPanel::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < 7; ++i)
        if (tileBounds (i).contains (e.getPosition())) { select (i); return; }
}

void MainPanel::paint (Graphics& g)
{
    g.drawImage (texture, getLocalBounds().toFloat());

    // header
    g.setGradientFill (ColourGradient (Colour (0xffffe0a8), 0, 14, col::gold.darker (0.2f), 0, 46, false));
    g.setFont (uiFont (30.0f, true, 0.14f));
    g.drawText ("VOCAL STRIP", 28, 10, 360, 36, Justification::centredLeft, false);
    g.setColour (col::textDim);
    g.setFont (uiFont (12.5f, true, 0.2f));
    g.drawText ("ROHIT JI  |  VOCAL CHANNEL STRIP", 30, 44, 420, 18, Justification::centredLeft, false);
    g.drawText ("PRESET", W - 24 - 280 - 80, 18, 70, 32, Justification::centredRight, false);

    // chain tiles
    for (int i = 0; i < 7; ++i)
    {
        auto t = tileBounds (i).toFloat();
        const auto c = moduleColour (i);
        const bool sel = i == current;
        const bool on = proc.value (tileOnIds[i]) > 0.5f;
        g.setGradientFill (ColourGradient (sel ? c.withAlpha (0.28f) : Colour (0xff1f2229), 0, t.getY(), sel ? c.withAlpha (0.08f) : Colour (0xff15171c), 0, t.getBottom(), false));
        g.fillRoundedRectangle (t, 8.0f);
        g.setColour (sel ? c : col::edge);
        g.drawRoundedRectangle (t.reduced (0.5f), 8.0f, sel ? 2.0f : 1.0f);
        g.setColour (on ? c : c.withAlpha (0.3f));
        g.fillRoundedRectangle (t.getX() + 10, t.getY() + 10, 4, t.getHeight() - 20, 2.0f);
        g.setColour (on ? col::text : col::textDim.withAlpha (0.6f));
        g.setFont (uiFont (15.0f, true, 0.08f));
        g.drawText (tileNames[i], t.withTrimmedLeft (22).withHeight (40), Justification::centredLeft, false);

        String status = on ? "ON" : "OFF";
        float gr = 0.0f;
        if (i == 1) gr = cGr; else if (i == 2) gr = dGr; else if (i == 4) gr = gGr;
        if (on && (i == 1 || i == 2 || i == 4)) status = "GR " + String (gr, 1);
        g.setFont (uiFont (12.0f, true));
        g.setColour (on ? c.brighter (0.2f) : col::textDim.withAlpha (0.5f));
        g.drawText (status, t.withTrimmedLeft (22).withTrimmedTop (38).withHeight (20), Justification::centredLeft, false);
        if (i < 6)
        {
            g.setColour (col::textDim.withAlpha (0.5f));
            g.drawText (">", Rectangle<float> (t.getRight(), t.getY(), 10, t.getHeight()), Justification::centred, false);
        }
    }

    // side meters
    {
        auto m = Rectangle<int> (sideArea.getX(), sideArea.getY() + 320, sideArea.getWidth(), sideArea.getHeight() - 320).toFloat();
        g.setColour (Colour (0xff15171c));
        g.fillRoundedRectangle (m, 8.0f);
        g.setColour (col::edge);
        g.drawRoundedRectangle (m.reduced (0.5f), 8.0f, 1.0f);
        auto inner = m.reduced (14, 12);
        auto labels = inner.removeFromBottom (20);
        const float bw = 26.0f;
        auto drawBar = [&] (float x, float db, const String& name)
        {
            auto b = Rectangle<float> (x, inner.getY(), bw, inner.getHeight());
            g.setColour (col::screen);
            g.fillRoundedRectangle (b, 4.0f);
            const float f = jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            auto fill = b.withTop (b.getBottom() - b.getHeight() * f);
            g.setGradientFill (ColourGradient (Colour (0xffff4d5e), 0, b.getY(), Colour (0xff7cf0b0), 0, b.getBottom(), false));
            g.fillRoundedRectangle (fill, 4.0f);
            g.setColour (col::textDim);
            g.setFont (uiFont (12.0f, true));
            g.drawText (name, Rectangle<float> (x - 8, labels.getY(), bw + 16, 20), Justification::centred, false);
        };
        drawBar (inner.getX() + 12, inLvl, "IN");
        drawBar (inner.getRight() - 12 - bw, outLvl, "OUT");
        g.setColour (col::textDim.withAlpha (0.7f));
        g.setFont (uiFont (10.5f));
        for (int db : { 0, -12, -24, -36, -48 })
        {
            const float y = inner.getBottom() - inner.getHeight() * (float) (db + 60) / 60.0f;
            g.drawText (String (db), Rectangle<float> (inner.getCentreX() - 15, y - 7, 30, 14), Justification::centred, false);
        }
    }

    g.setColour (col::textDim.withAlpha (0.8f));
    g.setFont (uiFont (11.5f, true, 0.18f));
    g.drawText ("EQ  >  COMP  >  DE-ESS  >  SATURATE  >  GLUE  >  DELAY  >  REVERB", 26, H - 26, 700, 18, Justification::centredLeft, false);
    g.drawText ("v1.0  |  ROHIT JI", W - 26 - 300, H - 26, 300, 18, Justification::centredRight, false);
}

void MainPanel::timerCallback()
{
    auto sm = [] (float& v, float t) { v = t > v ? t : v + (t - v) * 0.25f; };
    sm (inLvl, proc.inLevel.load()); sm (outLvl, proc.outLevel.load());
    cGr += (proc.compGr.load() - cGr) * 0.3f;
    dGr += (proc.deessGr.load() - dGr) * 0.3f;
    gGr += (proc.glueGr.load() - gGr) * 0.3f;
    pages[current]->tick();
    repaint (Rectangle<int> (24, 76, 1012, 70));
    repaint (sideArea);
    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
}
} // namespace vsui

//==============================================================================
VocalStripEditor::VocalStripEditor (VocalStripProcessor& p) : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (vsui::MainPanel::W * 3 / 5, vsui::MainPanel::H * 3 / 5, vsui::MainPanel::W * 2, vsui::MainPanel::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) vsui::MainPanel::W / (double) vsui::MainPanel::H);
    setSize (vsui::MainPanel::W, vsui::MainPanel::H);
}

void VocalStripEditor::resized()
{
    panel.setBounds (0, 0, vsui::MainPanel::W, vsui::MainPanel::H);
    panel.setTransform (AffineTransform::scale ((float) getWidth() / (float) vsui::MainPanel::W));
}
