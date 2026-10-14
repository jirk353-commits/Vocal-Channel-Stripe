#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include "ReverbDSP.h"

namespace vs
{
using AC = juce::dsp::IIR::ArrayCoefficients<float>;
using Coeffs = juce::dsp::IIR::Coefficients<float>;

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept  { return 20.0f * std::log10 (std::max (g, 1.0e-6f)); }
inline float timeCoef (float ms, double fs) noexcept
{
    return (float) std::exp (-1.0 / (std::max (0.01, (double) ms) * 0.001 * fs));
}

/** Direct-form II transposed biquad, stereo, with in-place coefficient updates. */
struct Biquad2
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] {}, z2[2] {};
    void set (const std::array<float, 6>& c) noexcept
    {
        const float inv = 1.0f / c[3];
        b0 = c[0] * inv; b1 = c[1] * inv; b2 = c[2] * inv; a1 = c[4] * inv; a2 = c[5] * inv;
    }
    void setIdentity() noexcept { b0 = 1; b1 = b2 = a1 = a2 = 0; }
    void reset() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    float process (int ch, float x) noexcept
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
};

//==============================================================================
// EQ
//==============================================================================
enum EqType { Bell = 0, LowShelf, HighShelf, Notch, LowCut, HighCut, BandPass };
inline const juce::StringArray& eqTypeNames()
{
    static const juce::StringArray n { "Bell", "Low Shelf", "High Shelf", "Notch", "Low Cut", "High Cut", "Band Pass" };
    return n;
}

struct EqBandParams { bool on = false; int type = Bell; float freq = 1000, gain = 0, q = 0.7f; };

/** Coefficients for one band: up to two biquads (cuts are 24 dB/oct). */
inline int eqCoefficients (const EqBandParams& p, double fs, std::array<float, 6>& c1, std::array<float, 6>& c2)
{
    const float f = juce::jlimit (10.0f, (float) fs * 0.45f, p.freq);
    const float q = juce::jlimit (0.1f, 30.0f, p.q);
    const float g = dbToGain (p.gain);
    switch (p.type)
    {
        case LowShelf:  c1 = AC::makeLowShelf  (fs, f, juce::jmin (q, 2.0f), g); return 1;
        case HighShelf: c1 = AC::makeHighShelf (fs, f, juce::jmin (q, 2.0f), g); return 1;
        case Notch:     c1 = AC::makeNotch (fs, f, q); return 1;
        case BandPass:  c1 = AC::makeBandPass (fs, f, q); return 1;
        case LowCut:    c1 = AC::makeHighPass (fs, f, 0.5412f * q / 0.7071f); c2 = AC::makeHighPass (fs, f, 1.3066f); return 2;
        case HighCut:   c1 = AC::makeLowPass  (fs, f, 0.5412f * q / 0.7071f); c2 = AC::makeLowPass  (fs, f, 1.3066f); return 2;
        default:        c1 = AC::makePeakFilter (fs, f, q, g); return 1;
    }
}

class Equalizer
{
public:
    static constexpr int numBands = 8;
    void prepare (double sampleRate) { fs = sampleRate; for (auto& b : bands) { b.f1.reset(); b.f2.reset(); b.last = {}; b.last.freq = -1; } }

    void setBand (int i, const EqBandParams& p)
    {
        auto& b = bands[(size_t) i];
        if (p.on == b.last.on && p.type == b.last.type && p.freq == b.last.freq && p.gain == b.last.gain && p.q == b.last.q)
            return;
        if (p.type != b.last.type) { b.f1.reset(); b.f2.reset(); }
        b.last = p;
        std::array<float, 6> c1 {}, c2 {};
        b.stages = eqCoefficients (p, fs, c1, c2);
        b.f1.set (c1);
        if (b.stages > 1) b.f2.set (c2);
    }

    float process (int ch, float x) noexcept
    {
        for (auto& b : bands)
            if (b.last.on)
            {
                x = b.f1.process (ch, x);
                if (b.stages > 1) x = b.f2.process (ch, x);
            }
        return x;
    }

private:
    struct Band { Biquad2 f1, f2; int stages = 1; EqBandParams last; };
    std::array<Band, numBands> bands;
    double fs = 48000;
};

//==============================================================================
// Compressor with OPTO / VCA / FET character
//==============================================================================
inline float kneeGain (float x, float thr, float ratio, float knee) noexcept
{
    const float over = x - thr;
    const float slope = 1.0f / ratio - 1.0f;
    if (2.0f * over <= -knee) return 0.0f;
    if (2.0f * over < knee)   return slope * (over + 0.5f * knee) * (over + 0.5f * knee) / (2.0f * knee);
    return slope * over;
}

class Compressor
{
public:
    enum Model { Opto = 0, Vca, Fet };
    struct Params { int model = Vca; float thr = -18, ratio = 4, attackMs = 10, releaseMs = 120, makeup = 0, mix = 1; };

    void prepare (double sampleRate) { fs = sampleRate; reset(); }
    void reset() { env = 0; gr = 0; s1 = s2 = 0; memory = 0; prevGain = 1; colourLp[0] = colourLp[1] = 0; }
    void setParams (const Params& np)
    {
        p = np;
        float atk = p.attackMs, rel = p.releaseMs;
        switch (p.model)
        {
            case Opto: atk = juce::jmax (5.0f, atk * 1.5f); knee = 10.0f; rmsMs = 10.0f; break;
            case Fet:  atk = juce::jmax (0.02f, atk * 0.1f); rel *= 0.6f; knee = 2.0f; rmsMs = 0.0f; break;
            default:   knee = 4.0f; rmsMs = 5.0f; break;
        }
        aAtt = timeCoef (atk, fs);
        aRel = timeCoef (rel, fs);
        aRms = rmsMs > 0 ? 1.0f - timeCoef (rmsMs, fs) : 1.0f;
        aMemUp = 1.0f - timeCoef (2500.0f, fs);
        aMemDn = 1.0f - timeCoef (6000.0f, fs);
        makeupG = dbToGain (p.makeup);
    }

    void process (float& l, float& r) noexcept
    {
        // detector (FET listens to its own output = feedback style)
        float dl = l, dr = r;
        if (p.model == Fet) { dl *= prevGain; dr *= prevGain; }
        const float pk = juce::jmax (dl * dl, dr * dr);
        env += aRms * (pk - env);
        const float lvl = 10.0f * std::log10 (env + 1.0e-12f) + (p.model == Fet ? 0.0f : 3.0f);
        const float target = -kneeGain (lvl, p.thr, p.ratio, knee);    // positive dB of reduction

        if (p.model == Opto)
        {
            memory += target > 1.5f ? aMemUp * (1.0f - memory) : aMemDn * (0.0f - memory);
            const float slowCoef = timeCoef (p.releaseMs * (4.0f + 20.0f * memory), fs);
            s1 = target > s1 ? aAtt * s1 + (1 - aAtt) * target : aRel * s1 + (1 - aRel) * target;
            s2 = target > s2 ? aAtt * s2 + (1 - aAtt) * target : slowCoef * s2 + (1 - slowCoef) * target;
            gr = 0.5f * (s1 + s2);
        }
        else
            gr = target > gr ? aAtt * gr + (1 - aAtt) * target : aRel * gr + (1 - aRel) * target;

        const float g = dbToGain (-gr);
        prevGain = g;
        float yl = l * g, yr = r * g;

        // model colour
        if (p.model == Opto)      { yl += 0.25f * (tube (yl) - yl); yr += 0.25f * (tube (yr) - yr); }
        else if (p.model == Fet)  { yl += 0.35f * (std::tanh (yl * 1.3f) / 1.3f - yl); yr += 0.35f * (std::tanh (yr * 1.3f) / 1.3f - yr); }

        yl *= makeupG; yr *= makeupG;
        l = l + p.mix * (yl - l);
        r = r + p.mix * (yr - r);
    }

    float getGr() const noexcept { return gr; }

private:
    static float tube (float x) noexcept
    {
        constexpr float b = 0.2f;
        static const float tb = std::tanh (b), n = 1.0f / (1.0f - tb * tb);
        return (std::tanh (x + b) - tb) * n;
    }
    double fs = 48000;
    Params p;
    float knee = 4, rmsMs = 5, aAtt = 0, aRel = 0, aRms = 1, aMemUp = 0, aMemDn = 0, makeupG = 1;
    float env = 0, gr = 0, s1 = 0, s2 = 0, memory = 0, prevGain = 1, colourLp[2] {};
};

//==============================================================================
// De-esser (split band: only the "s" band is reduced)
//==============================================================================
class DeEsser
{
public:
    struct Params { float freq = 6000, thr = -30, range = 10; bool wide = false, listen = false; };
    void prepare (double sampleRate)
    {
        fs = sampleRate; det.reset(); env = 0; gr = 0; last.freq = -1;
        split.prepare ({ fs, 512, 2 });
        split.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        split.reset();
    }
    void setParams (const Params& np)
    {
        if (np.freq != last.freq)
        {
            split.setCutoffFrequency (juce::jmin (np.freq * 0.8f, (float) fs * 0.45f));
            det.set (AC::makeBandPass (fs, juce::jmin (np.freq * 1.2f, (float) fs * 0.45f), 1.2f));
        }
        last = np;
        aAtt = timeCoef (0.5f, fs); aRel = timeCoef (60.0f, fs);
    }
    void process (float& l, float& r) noexcept
    {
        const float bl = det.process (0, l), br = det.process (1, r);
        const float pk = juce::jmax (std::abs (bl), std::abs (br));
        env = pk > env ? aAtt * env + (1 - aAtt) * pk : aRel * env + (1 - aRel) * pk;
        const float lvl = gainToDb (env);
        const float target = juce::jlimit (0.0f, last.range, (lvl - last.thr) * 0.75f);
        gr += 0.05f * (target - gr);
        const float g = dbToGain (-gr);

        float ll, hl, lr, hr;
        split.processSample (0, l, ll, hl);
        split.processSample (1, r, lr, hr);
        if (last.listen) { l = hl; r = hr; return; }
        if (last.wide)   { l *= g; r *= g; return; }
        l = ll + hl * g;            // Linkwitz-Riley split: flat sum when g == 1
        r = lr + hr * g;
    }
    float getGr() const noexcept { return gr; }
private:
    double fs = 48000;
    juce::dsp::LinkwitzRileyFilter<float> split;
    Biquad2 det;
    Params last;
    float env = 0, gr = 0, aAtt = 0, aRel = 0;
};

//==============================================================================
// 3-band saturation + Air + Mid-tone exciter
//==============================================================================
class Saturator
{
public:
    struct Params { int type = 0; float low = 0, mid = 0, high = 0, xLow = 250, xHigh = 4000, air = 0, tone = 0, out = 0; };
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        juce::dsp::ProcessSpec spec { fs, 512, 2 };
        for (auto* f : { &xl, &xh, &apLow }) { f->prepare (spec); f->reset(); }
        xl.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        xh.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        apLow.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
        airHp.reset(); toneBp.reset(); airShelf.reset(); toneBell.reset();
        airHp.set (AC::makeHighPass (fs, juce::jmin (7000.0f, (float) fs * 0.45f), 0.7071f));
        toneBp.set (AC::makeBandPass (fs, 2500.0f, 0.9f));
        lastAir = lastTone = -1; lastXl = lastXh = -1;
    }
    void setParams (const Params& np)
    {
        p = np;
        if (p.xLow != lastXl) { lastXl = p.xLow; xl.setCutoffFrequency (p.xLow); }
        const float xhF = juce::jmax (p.xHigh, p.xLow * 1.5f);
        if (xhF != lastXh) { lastXh = xhF; xh.setCutoffFrequency (xhF); apLow.setCutoffFrequency (xhF); }
        if (p.air != lastAir)
        {
            lastAir = p.air;
            airShelf.set (AC::makeHighShelf (fs, juce::jmin (11000.0f, (float) fs * 0.45f), 0.7f, dbToGain (p.air * 0.05f)));
        }
        if (p.tone != lastTone)
        {
            lastTone = p.tone;
            toneBell.set (AC::makePeakFilter (fs, 2200.0f, 0.8f, dbToGain (p.tone * 0.04f)));
        }
        for (int b = 0; b < 3; ++b)
        {
            const float d = b == 0 ? p.low : b == 1 ? p.mid : p.high;
            drive[b] = dbToGain (d * 0.20f);                       // up to +20 dB into the shaper
            comp[b] = 1.0f / std::sqrt (drive[b]);                 // partial level compensation
            active[b] = d > 0.05f;
        }
        outG = dbToGain (p.out);
    }
    float process (int ch, float x) noexcept
    {
        float low, rest, mid, high;
        xl.processSample (ch, x, low, rest);
        xh.processSample (ch, rest, mid, high);
        low = apLow.processSample (ch, low);
        float bands[3] = { low, mid, high };
        for (int b = 0; b < 3; ++b)
            if (active[b])
                bands[b] = shape (bands[b] * drive[b]) * comp[b];
        float y = bands[0] + bands[1] + bands[2];

        // exciters
        if (p.tone > 0.05f)
        {
            const float t = toneBp.process (ch, y);
            y += (p.tone * 0.01f) * 0.6f * std::tanh (t * 4.0f) * 0.5f;
            y = toneBell.process (ch, y);
        }
        if (p.air > 0.05f)
        {
            const float h = airHp.process (ch, y);
            y += (p.air * 0.01f) * 0.7f * std::tanh (h * 4.0f) * 0.5f;
            y = airShelf.process (ch, y);
        }
        return y * outG;
    }
private:
    float shape (float x) const noexcept
    {
        switch (p.type)
        {
            case 1: { const float a = std::abs (x); return x / std::cbrt (1.0f + a * a * a); }    // tape
            case 2: return std::tanh (x);                                                          // warm
            default:                                                                               // tube
            {
                constexpr float b = 0.25f;
                static const float tb = std::tanh (b), n = 1.0f / (1.0f - tb * tb);
                return (std::tanh (x + b) - tb) * n;
            }
        }
    }
    double fs = 48000;
    Params p;
    juce::dsp::LinkwitzRileyFilter<float> xl, xh, apLow;
    Biquad2 airHp, toneBp, airShelf, toneBell;
    float drive[3] { 1, 1, 1 }, comp[3] { 1, 1, 1 }, outG = 1;
    bool active[3] {};
    float lastAir = -1, lastTone = -1, lastXl = -1, lastXh = -1;
};

//==============================================================================
// Delay: Mono / Stereo / Ping-Pong, tempo sync, filtered feedback, ducking
//==============================================================================
class Delay
{
public:
    enum Mode { Mono = 0, Stereo, PingPong };
    struct Params { int mode = PingPong; float timeMs = 375, feedback = 35, lowCut = 200, highCut = 6000, width = 100, duck = 0; };

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (auto& b : buf) b.prepare ((int) (2.6 * fs));
        for (auto& b : buf) b.clear();
        hp.reset(); lp.reset(); curT = -1; duckEnv = 0; lastLc = lastHc = -1;
    }
    void setParams (const Params& np)
    {
        p = np;
        if (p.lowCut != lastLc)  { lastLc = p.lowCut;  hp.set (AC::makeHighPass (fs, p.lowCut, 0.7071f)); }
        if (p.highCut != lastHc) { lastHc = p.highCut; lp.set (AC::makeLowPass (fs, juce::jmin (p.highCut, (float) fs * 0.45f), 0.7071f)); }
        targetT = juce::jlimit (1.0f, 2.5f * (float) fs, p.timeMs * 0.001f * (float) fs);
        if (curT < 0) curT = targetT;
        fb = juce::jlimit (0.0f, 0.95f, p.feedback * 0.01f);
        aDuckRel = timeCoef (250.0f, fs);
    }
    /** returns the wet signal only */
    void process (float inL, float inR, float& outL, float& outR) noexcept
    {
        curT += 0.0005f * (targetT - curT);
        const float dL = buf[0].read (curT);
        const float tR = p.mode == Stereo ? curT * 1.0f : curT;
        const float dR = buf[1].read (tR);

        float fL = lp.process (0, hp.process (0, dL));
        float fR = lp.process (1, hp.process (1, dR));

        switch (p.mode)
        {
            case Mono:
            {
                const float m = 0.5f * (inL + inR);
                buf[0].push (m + fL * fb); buf[1].push (m + fL * fb);
                outL = outR = dL;
                break;
            }
            case Stereo:
                buf[0].push (inL + fL * fb); buf[1].push (inR + fR * fb);
                outL = dL; outR = dR;
                break;
            default: // ping-pong: mono input enters the left line, the lines feed each other
            {
                const float m = 0.5f * (inL + inR);
                buf[0].push (m + fR * fb); buf[1].push (fL * fb);
                outL = dL; outR = dR;
                break;
            }
        }
        // width
        const float w = p.width * 0.01f;
        const float mid = 0.5f * (outL + outR), side = 0.5f * (outL - outR) * w;
        outL = mid + side; outR = mid - side;

        // ducking: wet drops while the vocal is singing
        if (p.duck > 0.5f)
        {
            const float lvl = juce::jmax (std::abs (inL), std::abs (inR));
            duckEnv = lvl > duckEnv ? lvl : aDuckRel * duckEnv + (1 - aDuckRel) * lvl;
            const float g = 1.0f - juce::jlimit (0.0f, 1.0f, duckEnv * 4.0f) * p.duck * 0.01f;
            outL *= g; outR *= g;
        }
    }
private:
    double fs = 48000;
    Params p;
    std::array<gv::Delay, 2> buf;
    Biquad2 hp, lp;
    float curT = -1, targetT = 1000, fb = 0.3f, duckEnv = 0, aDuckRel = 0, lastLc = -1, lastHc = -1;
};

//==============================================================================
// Glue compressor (bus style, stepped ratio / attack / release, auto release)
//==============================================================================
class Glue
{
public:
    struct Params { float thr = -10; int ratio = 0, attack = 2, release = 4; float makeup = 0, mix = 1; };
    static const juce::StringArray& ratioNames()   { static const juce::StringArray n { "2:1", "4:1", "10:1" }; return n; }
    static const juce::StringArray& attackNames()  { static const juce::StringArray n { "0.1", "0.3", "1", "3", "10", "30" }; return n; }
    static const juce::StringArray& releaseNames() { static const juce::StringArray n { "0.1", "0.3", "0.6", "1.2", "AUTO" }; return n; }

    void prepare (double sampleRate) { fs = sampleRate; gr = 0; fast = slow = 0; }
    void setParams (const Params& np)
    {
        p = np;
        static const float ratios[] = { 2, 4, 10 }, atk[] = { 0.1f, 0.3f, 1, 3, 10, 30 }, rel[] = { 100, 300, 600, 1200, 0 };
        ratio = ratios[juce::jlimit (0, 2, p.ratio)];
        aAtt = timeCoef (atk[juce::jlimit (0, 5, p.attack)], fs);
        autoRel = p.release >= 4;
        aRel = timeCoef (autoRel ? 150.0f : rel[p.release], fs);
        aFast = timeCoef (100.0f, fs); aSlow = timeCoef (1500.0f, fs);
        makeupG = dbToGain (p.makeup);
    }
    void process (float& l, float& r) noexcept
    {
        const float pk = juce::jmax (std::abs (l), std::abs (r));
        const float target = -kneeGain (gainToDb (pk), p.thr, ratio, 2.0f);
        if (autoRel)
        {
            fast = target > fast ? aAtt * fast + (1 - aAtt) * target : aFast * fast + (1 - aFast) * target;
            slow = target > slow ? aAtt * slow + (1 - aAtt) * target : aSlow * slow + (1 - aSlow) * target;
            gr = juce::jmax (fast, slow * 0.7f);
        }
        else
            gr = target > gr ? aAtt * gr + (1 - aAtt) * target : aRel * gr + (1 - aRel) * target;
        const float g = dbToGain (-gr) * makeupG;
        l = l + p.mix * (l * g - l);
        r = r + p.mix * (r * g - r);
    }
    float getGr() const noexcept { return gr; }
private:
    double fs = 48000;
    Params p;
    float ratio = 2, aAtt = 0, aRel = 0, aFast = 0, aSlow = 0, makeupG = 1, gr = 0, fast = 0, slow = 0;
    bool autoRel = true;
};

} // namespace vs
