#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
namespace
{
    struct EqDefault { bool on; int type; float freq, gain, q; };
    const EqDefault eqDefaults[8] =
    {
        { true,  vs::LowCut,    80.0f, 0.0f, 0.71f },
        { false, vs::LowShelf, 150.0f, 0.0f, 0.71f },
        { false, vs::Bell,     300.0f, 0.0f, 1.0f },
        { false, vs::Bell,     800.0f, 0.0f, 1.0f },
        { false, vs::Bell,    2500.0f, 0.0f, 1.0f },
        { false, vs::Bell,    5000.0f, 0.0f, 1.0f },
        { false, vs::HighShelf, 10000.0f, 0.0f, 0.71f },
        { false, vs::HighCut, 18000.0f, 0.0f, 0.71f },
    };

    struct Preset { const char* name; std::vector<std::pair<String, float>> values; };

    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> list =
        {
            { "Init", {} },
            { "Pop Vocal", {
                { "eq1_freq", 100 }, { "eq3_on", 1 }, { "eq3_freq", 300 }, { "eq3_gain", -2.5f }, { "eq3_q", 1.2f },
                { "eq5_on", 1 }, { "eq5_freq", 3000 }, { "eq5_gain", 2 }, { "eq7_on", 1 }, { "eq7_gain", 2.5f },
                { "comp_model", 1 }, { "comp_thresh", -20 }, { "comp_ratio", 4 }, { "comp_attack", 8 }, { "comp_release", 100 }, { "comp_makeup", 5 },
                { "ds_on", 1 }, { "ds_thresh", -32 }, { "sat_on", 1 }, { "sat_mid", 15 }, { "sat_air", 30 },
                { "dly_on", 1 }, { "dly_mix", 12 }, { "rev_on", 1 }, { "rev_type", 4 }, { "rev_mix", 15 }, { "glue_on", 1 } } },
            { "Bhojpuri Lead", {
                { "eq1_freq", 110 }, { "eq3_on", 1 }, { "eq3_freq", 350 }, { "eq3_gain", -3 }, { "eq5_on", 1 }, { "eq5_freq", 2800 }, { "eq5_gain", 3 },
                { "eq7_on", 1 }, { "eq7_gain", 3 }, { "comp_model", 2 }, { "comp_thresh", -22 }, { "comp_ratio", 6 }, { "comp_attack", 5 }, { "comp_release", 80 }, { "comp_makeup", 6 },
                { "ds_on", 1 }, { "ds_thresh", -30 }, { "ds_range", 12 }, { "sat_on", 1 }, { "sat_mid", 25 }, { "sat_tone", 30 }, { "sat_air", 25 },
                { "dly_on", 1 }, { "dly_mode", 2 }, { "dly_div", 6 }, { "dly_mix", 15 }, { "dly_duck", 50 },
                { "rev_on", 1 }, { "rev_type", 5 }, { "rev_decay", 2.4f }, { "rev_mix", 18 }, { "glue_on", 1 }, { "glue_thresh", -12 } } },
            { "Devotional Warm", {
                { "eq1_freq", 90 }, { "eq2_on", 1 }, { "eq2_freq", 180 }, { "eq2_gain", 1.5f }, { "eq6_on", 1 }, { "eq6_freq", 6000 }, { "eq6_gain", -1.5f },
                { "comp_model", 0 }, { "comp_thresh", -24 }, { "comp_ratio", 3 }, { "comp_makeup", 5 },
                { "ds_on", 1 }, { "sat_on", 1 }, { "sat_type", 0 }, { "sat_low", 15 }, { "sat_mid", 20 },
                { "rev_on", 1 }, { "rev_type", 5 }, { "rev_decay", 3.2f }, { "rev_pre", 35 }, { "rev_mix", 22 }, { "glue_on", 1 } } },
            { "Rap Vocal", {
                { "eq1_freq", 120 }, { "eq3_on", 1 }, { "eq3_freq", 400 }, { "eq3_gain", -3 }, { "eq5_on", 1 }, { "eq5_freq", 4000 }, { "eq5_gain", 3 },
                { "comp_model", 2 }, { "comp_thresh", -25 }, { "comp_ratio", 8 }, { "comp_attack", 2 }, { "comp_release", 60 }, { "comp_makeup", 8 },
                { "ds_on", 1 }, { "ds_thresh", -28 }, { "sat_on", 1 }, { "sat_type", 1 }, { "sat_mid", 30 }, { "sat_high", 15 },
                { "dly_on", 1 }, { "dly_mode", 0 }, { "dly_div", 3 }, { "dly_mix", 8 }, { "glue_on", 1 }, { "glue_thresh", -14 }, { "glue_ratio", 1 } } },
            { "Airy Female", {
                { "eq1_freq", 130 }, { "eq4_on", 1 }, { "eq4_freq", 900 }, { "eq4_gain", -2 }, { "eq7_on", 1 }, { "eq7_freq", 12000 }, { "eq7_gain", 4 },
                { "comp_model", 0 }, { "comp_thresh", -22 }, { "comp_makeup", 5 }, { "ds_on", 1 }, { "ds_freq", 7500 }, { "ds_thresh", -34 },
                { "sat_on", 1 }, { "sat_air", 55 }, { "sat_high", 10 }, { "rev_on", 1 }, { "rev_type", 4 }, { "rev_mix", 20 },
                { "dly_on", 1 }, { "dly_mix", 10 }, { "glue_on", 1 } } },
            { "Deep Male", {
                { "eq1_freq", 70 }, { "eq2_on", 1 }, { "eq2_freq", 140 }, { "eq2_gain", 2 }, { "eq3_on", 1 }, { "eq3_freq", 450 }, { "eq3_gain", -2.5f },
                { "comp_model", 1 }, { "comp_thresh", -20 }, { "comp_ratio", 3 }, { "comp_makeup", 4 }, { "sat_on", 1 }, { "sat_low", 25 }, { "sat_type", 1 },
                { "rev_on", 1 }, { "rev_type", 3 }, { "rev_mix", 14 }, { "glue_on", 1 } } },
            { "Podcast Voice", {
                { "eq1_freq", 90 }, { "eq3_on", 1 }, { "eq3_freq", 300 }, { "eq3_gain", -2 }, { "eq5_on", 1 }, { "eq5_freq", 3500 }, { "eq5_gain", 2 },
                { "comp_model", 1 }, { "comp_thresh", -24 }, { "comp_ratio", 4 }, { "comp_makeup", 7 }, { "ds_on", 1 }, { "glue_on", 1 }, { "glue_ratio", 1 } } },
            { "Backing Vocals", {
                { "eq1_freq", 180 }, { "eq7_on", 1 }, { "eq7_gain", 2 }, { "comp_model", 1 }, { "comp_thresh", -26 }, { "comp_ratio", 6 }, { "comp_makeup", 6 },
                { "ds_on", 1 }, { "ds_range", 14 }, { "dly_on", 1 }, { "dly_mode", 1 }, { "dly_mix", 15 },
                { "rev_on", 1 }, { "rev_type", 5 }, { "rev_mix", 30 }, { "glue_on", 1 } } },
            { "Lo-Fi Vocal", {
                { "eq1_freq", 300 }, { "eq8_on", 1 }, { "eq8_freq", 5000 }, { "eq4_on", 1 }, { "eq4_freq", 1200 }, { "eq4_gain", 4 },
                { "comp_model", 2 }, { "comp_thresh", -24 }, { "comp_ratio", 10 }, { "comp_makeup", 6 }, { "sat_on", 1 }, { "sat_type", 1 }, { "sat_mid", 45 },
                { "dly_on", 1 }, { "dly_mode", 0 }, { "dly_mix", 15 }, { "rev_on", 1 }, { "rev_type", 8 }, { "rev_mix", 18 } } },
        };
        return list;
    }

    NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }
}

const StringArray& VocalStripProcessor::delayDivisions()
{
    static const StringArray d { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" };
    return d;
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout VocalStripProcessor::createLayout()
{
    using F = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;

    auto num = [] (int dec, String unit = {})
    {
        return A().withLabel (unit).withStringFromValueFunction ([dec, unit] (float x, int)
                   { return String (x, dec) + (unit.isEmpty() ? "" : " " + unit); });
    };
    auto db = A().withLabel ("dB").withStringFromValueFunction ([] (float x, int) { return (x > 0.05f ? "+" : "") + String (x, 1) + " dB"; });
    auto pct = A().withLabel ("%").withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x)) + "%"; });
    auto hz = A().withLabel ("Hz").withStringFromValueFunction ([] (float x, int)
                  { return x >= 1000.0f ? String (x / 1000.0f, x >= 10000.0f ? 1 : 2) + " k" : String (roundToInt (x)) + " Hz"; });
    auto ms = A().withLabel ("ms").withStringFromValueFunction ([] (float x, int)
                  { return x < 10.0f ? String (x, 1) + " ms" : String (roundToInt (x)) + " ms"; });
    auto onOff = [&] (const String& id, const String& name, bool def)
    { v.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto choice = [&] (const String& id, const String& name, const StringArray& c, int def)
    { v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, c, def)); };
    auto flt = [&] (const String& id, const String& name, NormalisableRange<float> r, float def, A a)
    { v.push_back (std::make_unique<F> (ParameterID { id, 1 }, name, r, def, a)); };

    flt ("in_gain", "Input Gain", NormalisableRange<float> (-24, 24, 0.1f), 0, db);
    flt ("out_gain", "Output Gain", NormalisableRange<float> (-24, 24, 0.1f), 0, db);

    // EQ
    onOff ("eq_on", "EQ On", true);
    for (int b = 0; b < 8; ++b)
    {
        const auto& d = eqDefaults[b];
        const String n = "EQ " + String (b + 1) + " ";
        onOff (eqId (b, "on"), n + "On", d.on);
        choice (eqId (b, "type"), n + "Type", vs::eqTypeNames(), d.type);
        flt (eqId (b, "freq"), n + "Freq", skewed (20, 20000, 1000), d.freq, hz);
        flt (eqId (b, "gain"), n + "Gain", NormalisableRange<float> (-24, 24, 0.1f), d.gain, db);
        flt (eqId (b, "q"), n + "Q", skewed (0.1f, 18.0f, 1.0f), d.q, num (2));
    }

    // Compressor
    onOff ("comp_on", "Comp On", true);
    choice ("comp_model", "Comp Model", { "Opto", "VCA", "FET" }, 1);
    flt ("comp_thresh", "Comp Threshold", NormalisableRange<float> (-60, 0, 0.1f), -18, db);
    flt ("comp_ratio", "Comp Ratio", skewed (1, 20, 4), 4, A().withStringFromValueFunction ([] (float x, int) { return String (x, 1) + ":1"; }));
    flt ("comp_attack", "Comp Attack", skewed (0.1f, 100, 10), 10, ms);
    flt ("comp_release", "Comp Release", skewed (10, 1500, 150), 120, ms);
    flt ("comp_makeup", "Comp Makeup", NormalisableRange<float> (-6, 24, 0.1f), 3, db);
    flt ("comp_mix", "Comp Mix", NormalisableRange<float> (0, 100), 100, pct);

    // De-esser
    onOff ("ds_on", "De-Ess On", false);
    flt ("ds_freq", "De-Ess Freq", skewed (2000, 12000, 6000), 6000, hz);
    flt ("ds_thresh", "De-Ess Threshold", NormalisableRange<float> (-60, 0, 0.1f), -30, db);
    flt ("ds_range", "De-Ess Range", NormalisableRange<float> (0, 24, 0.1f), 10, db);
    choice ("ds_mode", "De-Ess Mode", { "Split", "Wide" }, 0);
    onOff ("ds_listen", "De-Ess Listen", false);

    // Saturation
    onOff ("sat_on", "Sat On", false);
    choice ("sat_type", "Sat Type", { "Tube", "Tape", "Warm" }, 0);
    flt ("sat_low", "Sat Low Drive", NormalisableRange<float> (0, 100), 0, pct);
    flt ("sat_mid", "Sat Mid Drive", NormalisableRange<float> (0, 100), 20, pct);
    flt ("sat_high", "Sat High Drive", NormalisableRange<float> (0, 100), 0, pct);
    flt ("sat_xlow", "Sat Low/Mid", skewed (80, 1000, 250), 250, hz);
    flt ("sat_xhigh", "Sat Mid/High", skewed (1500, 12000, 4000), 4000, hz);
    flt ("sat_tone", "Sat Mid Tone", NormalisableRange<float> (0, 100), 0, pct);
    flt ("sat_air", "Sat Air", NormalisableRange<float> (0, 100), 20, pct);
    flt ("sat_out", "Sat Output", NormalisableRange<float> (-12, 12, 0.1f), 0, db);

    // Delay
    onOff ("dly_on", "Delay On", false);
    choice ("dly_mode", "Delay Mode", { "Mono", "Stereo", "Ping-Pong" }, 2);
    onOff ("dly_sync", "Delay Sync", true);
    flt ("dly_time", "Delay Time", skewed (10, 2000, 300), 375, ms);
    choice ("dly_div", "Delay Division", delayDivisions(), 5);
    flt ("dly_fb", "Delay Feedback", NormalisableRange<float> (0, 95), 30, pct);
    flt ("dly_lowcut", "Delay Low Cut", skewed (20, 2000, 300), 250, hz);
    flt ("dly_highcut", "Delay High Cut", skewed (1000, 20000, 6000), 6000, hz);
    flt ("dly_width", "Delay Width", NormalisableRange<float> (0, 150), 100, pct);
    flt ("dly_duck", "Delay Duck", NormalisableRange<float> (0, 100), 30, pct);
    flt ("dly_mix", "Delay Level", NormalisableRange<float> (0, 100), 15, pct);

    // Reverb
    onOff ("rev_on", "Reverb On", false);
    StringArray envs;
    for (auto& t : gv::envTypes()) envs.add (t.name);
    choice ("rev_type", "Reverb Type", envs, 4);
    flt ("rev_decay", "Reverb Decay", skewed (0.2f, 12, 2.0f), 2.0f, num (2, "s"));
    flt ("rev_pre", "Reverb Pre-Delay", NormalisableRange<float> (0, 200), 20, ms);
    flt ("rev_size", "Reverb Size", NormalisableRange<float> (0, 100), 50, pct);
    flt ("rev_lowcut", "Reverb Low Cut", skewed (20, 1000, 200), 250, hz);
    flt ("rev_highcut", "Reverb High Cut", skewed (1000, 20000, 6000), 10000, hz);
    flt ("rev_mix", "Reverb Level", NormalisableRange<float> (0, 100), 18, pct);

    // Glue
    onOff ("glue_on", "Glue On", false);
    flt ("glue_thresh", "Glue Threshold", NormalisableRange<float> (-40, 0, 0.1f), -10, db);
    choice ("glue_ratio", "Glue Ratio", vs::Glue::ratioNames(), 0);
    choice ("glue_attack", "Glue Attack", vs::Glue::attackNames(), 3);
    choice ("glue_release", "Glue Release", vs::Glue::releaseNames(), 4);
    flt ("glue_makeup", "Glue Makeup", NormalisableRange<float> (-6, 15, 0.1f), 0, db);
    flt ("glue_mix", "Glue Mix", NormalisableRange<float> (0, 100), 100, pct);

    return { v.begin(), v.end() };
}

//==============================================================================
VocalStripProcessor::VocalStripProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<RangedAudioParameter*> (prm))
            cache[r->getParameterID().toStdString()] = apvts.getRawParameterValue (r->getParameterID());
}

bool VocalStripProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo()) && l.getMainInputChannelSet() == out;
}

void VocalStripProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    eq.prepare (sr); comp.prepare (sr); deess.prepare (sr); sat.prepare (sr);
    delay.prepare (sr); reverb.prepare (sr); glue.prepare (sr);
    revLow.reset(); revHigh.reset(); lastRevLow = lastRevHigh = -1;
    for (auto* s : { &inG, &outG, &dlyMix, &revMix }) s->reset (sr, 0.05);
    inG.setCurrentAndTargetValue (vs::dbToGain (p ("in_gain")));
    outG.setCurrentAndTargetValue (vs::dbToGain (p ("out_gain")));
    dlyMix.setCurrentAndTargetValue (p ("dly_mix") * 0.01f);
    revMix.setCurrentAndTargetValue (p ("rev_mix") * 0.01f);
}

void VocalStripProcessor::setParam (const String& id, float v)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
        prm->endChangeGesture();
    }
}

void VocalStripProcessor::applyReverbEnvironment (int type)
{
    const auto& t = gv::envTypes()[(size_t) jlimit (0, 8, type)];
    setParam ("rev_type", (float) type);
    setParam ("rev_decay", jmin (12.0f, t.defDecay));
    setParam ("rev_pre", t.defPredelay);
    setParam ("rev_size", t.defSize);
}

//==============================================================================
void VocalStripProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numCh = jmin (2, getTotalNumOutputChannels());
    if (n == 0 || numCh == 0) return;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm.store (*bpm);

    // ---- parameters ------------------------------------------------------
    const bool eqOn = p ("eq_on") > 0.5f, compOn = p ("comp_on") > 0.5f, dsOn = p ("ds_on") > 0.5f, satOn = p ("sat_on") > 0.5f;
    const bool dlyOn = p ("dly_on") > 0.5f, revOn = p ("rev_on") > 0.5f, glueOn = p ("glue_on") > 0.5f;

    for (int b = 0; b < 8; ++b)
    {
        vs::EqBandParams bp;
        bp.on = apvts.getRawParameterValue (eqId (b, "on"))->load() > 0.5f;
        bp.type = (int) apvts.getRawParameterValue (eqId (b, "type"))->load();
        bp.freq = apvts.getRawParameterValue (eqId (b, "freq"))->load();
        bp.gain = apvts.getRawParameterValue (eqId (b, "gain"))->load();
        bp.q = apvts.getRawParameterValue (eqId (b, "q"))->load();
        eq.setBand (b, bp);
    }
    comp.setParams ({ (int) p ("comp_model"), p ("comp_thresh"), p ("comp_ratio"), p ("comp_attack"), p ("comp_release"),
                      p ("comp_makeup"), p ("comp_mix") * 0.01f });
    deess.setParams ({ p ("ds_freq"), p ("ds_thresh"), p ("ds_range"), (int) p ("ds_mode") == 1, p ("ds_listen") > 0.5f });
    sat.setParams ({ (int) p ("sat_type"), p ("sat_low"), p ("sat_mid"), p ("sat_high"), p ("sat_xlow"), p ("sat_xhigh"),
                     p ("sat_air"), p ("sat_tone"), p ("sat_out") });

    float delayMs = p ("dly_time");
    if (p ("dly_sync") > 0.5f)
    {
        static const float beats[] = { 0.125f, 1.0f / 6.0f, 0.25f, 0.375f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f, 2.0f, 4.0f };
        delayMs = (float) (60000.0 / jlimit (30.0, 300.0, hostBpm.load())) * beats[jlimit (0, 11, (int) p ("dly_div"))];
    }
    delay.setParams ({ (int) p ("dly_mode"), jmin (delayMs, 2500.0f), p ("dly_fb"), p ("dly_lowcut"), p ("dly_highcut"),
                       p ("dly_width"), p ("dly_duck") });

    gv::Reverb::Params rp;
    rp.type = (int) p ("rev_type"); rp.decay = p ("rev_decay"); rp.predelayMs = p ("rev_pre"); rp.size = p ("rev_size");
    const auto& env = gv::envTypes()[(size_t) jlimit (0, 8, rp.type)];
    rp.diffusion = env.defDiffusion; rp.damping = env.defDamping; rp.mod = 50; rp.early = 50;
    reverb.setParams (rp);
    if (p ("rev_lowcut") != lastRevLow)   { lastRevLow = p ("rev_lowcut");   revLow.set (vs::AC::makeHighPass (sr, lastRevLow, 0.7071f)); }
    if (p ("rev_highcut") != lastRevHigh) { lastRevHigh = p ("rev_highcut"); revHigh.set (vs::AC::makeLowPass (sr, jmin (lastRevHigh, (float) sr * 0.45f), 0.7071f)); }

    glue.setParams ({ p ("glue_thresh"), (int) p ("glue_ratio"), (int) p ("glue_attack"), (int) p ("glue_release"),
                      p ("glue_makeup"), p ("glue_mix") * 0.01f });

    inG.setTargetValue (vs::dbToGain (p ("in_gain")));
    outG.setTargetValue (vs::dbToGain (p ("out_gain")));
    dlyMix.setTargetValue (p ("dly_mix") * 0.01f);
    revMix.setTargetValue (p ("rev_mix") * 0.01f);

    // ---- audio -----------------------------------------------------------
    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    float inPk = 0, outSq = 0, cGr = 0, dGr = 0, gGr = 0;

    int start1, size1, start2, size2;
    analyzerFifo.prepareToWrite (n, start1, size1, start2, size2);
    int written = 0;

    for (int i = 0; i < n; ++i)
    {
        const float ig = inG.getNextValue();
        float l = L[i] * ig, r = (R ? R[i] : L[i]) * ig;
        inPk = jmax (inPk, jmax (std::abs (l), std::abs (r)));

        if (eqOn)   { l = eq.process (0, l); r = eq.process (1, r); }
        {
            const float a = 0.5f * (l + r);
            if (written < size1) analyzerData[(size_t) (start1 + written)] = a;
            else if (written - size1 < size2) analyzerData[(size_t) (start2 + written - size1)] = a;
            ++written;
        }
        if (compOn) { comp.process (l, r); cGr = jmax (cGr, comp.getGr()); }
        if (dsOn)   { deess.process (l, r); dGr = jmax (dGr, deess.getGr()); }
        if (satOn)  { l = sat.process (0, l); r = sat.process (1, r); }
        if (glueOn) { glue.process (l, r); gGr = jmax (gGr, glue.getGr()); }

        // time effects (added as sends so the dry vocal stays at full level)
        const float dm = dlyMix.getNextValue(), rm = revMix.getNextValue();
        float wl = 0, wr = 0;
        if (dlyOn)
        {
            float dl, dr; delay.process (l, r, dl, dr);
            wl += dl * dm; wr += dr * dm;
        }
        if (revOn)
        {
            float rl, rr; reverb.process (l, r, rl, rr);
            rl = revHigh.process (0, revLow.process (0, rl));
            rr = revHigh.process (1, revLow.process (1, rr));
            wl += rl * rm; wr += rr * rm;
        }
        l += wl; r += wr;

        const float og = outG.getNextValue();
        l *= og; r *= og;
        outSq += 0.5f * (l * l + r * r);
        if (R) { L[i] = l; R[i] = r; } else L[i] = 0.5f * (l + r);
    }
    analyzerFifo.finishedWrite (jmin (written, size1 + size2));

    inLevel.store (Decibels::gainToDecibels (inPk, -100.0f));
    outLevel.store (Decibels::gainToDecibels (std::sqrt (outSq / (float) n) * 1.414f, -100.0f));
    compGr.store (compOn ? cGr : 0.0f);
    deessGr.store (dsOn ? dGr : 0.0f);
    glueGr.store (glueOn ? gGr : 0.0f);
}

//==============================================================================
int VocalStripProcessor::getNumPrograms() { return (int) presets().size(); }
const String VocalStripProcessor::getProgramName (int i) { return isPositiveAndBelow (i, getNumPrograms()) ? String (presets()[(size_t) i].name) : String(); }

void VocalStripProcessor::setCurrentProgram (int index)
{
    if (! isPositiveAndBelow (index, getNumPrograms())) return;
    currentProgram = index;
    // reset everything (except input/output gain) to defaults, then apply the preset values
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<RangedAudioParameter*> (prm))
        {
            const auto id = r->getParameterID();
            if (id == "in_gain" || id == "out_gain") continue;
            r->beginChangeGesture();
            r->setValueNotifyingHost (r->getDefaultValue());
            r->endChangeGesture();
        }
    for (auto& [id, val] : presets()[(size_t) index].values)
        setParam (id, val);
    if (index > 0)
    {
        // preset reverbs pick sensible space settings unless the preset sets them
        bool hasDecay = false;
        for (auto& kv : presets()[(size_t) index].values) hasDecay |= kv.first == "rev_decay";
        if (! hasDecay)
        {
            const auto& t = gv::envTypes()[(size_t) jlimit (0, 8, (int) value ("rev_type"))];
            setParam ("rev_decay", jmin (12.0f, t.defDecay));
            setParam ("rev_pre", t.defPredelay);
        }
    }
}

void VocalStripProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void VocalStripProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

AudioProcessorEditor* VocalStripProcessor::createEditor() { return new VocalStripEditor (*this); }

#if ! VS_TEST_BUILD
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VocalStripProcessor(); }
#endif
