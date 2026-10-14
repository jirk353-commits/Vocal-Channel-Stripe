// Offline test runner for the Vocal Channel Strip: DSP checks + screenshots of every page.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdio>

using namespace juce;

static int failures = 0;
static void check (bool ok, const String& what)
{
    std::printf ("[%s] %s\n", ok ? " OK " : "FAIL", what.toRawUTF8());
    if (! ok) ++failures;
}
static void setP (VocalStripProcessor& p, const String& id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    jassert (prm != nullptr);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}
static void allOff (VocalStripProcessor& p)
{
    for (auto* id : { "eq_on", "comp_on", "ds_on", "sat_on", "glue_on", "dly_on", "rev_on" }) setP (p, id, 0);
}
static std::vector<float> run (VocalStripProcessor& p, std::function<float (int)> in, int total, int block = 512)
{
    std::vector<float> out; out.reserve ((size_t) total);
    AudioBuffer<float> buf (2, block); MidiBuffer m;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = jmin (block, total - pos);
        AudioBuffer<float> b (buf.getArrayOfWritePointers(), 2, 0, n);
        for (int i = 0; i < n; ++i) { const float x = in (pos + i); b.setSample (0, i, x); b.setSample (1, i, x); }
        p.processBlock (b, m);
        for (int i = 0; i < n; ++i) out.push_back (b.getSample (0, i));
    }
    return out;
}
static float rms (const std::vector<float>& x, size_t a, size_t b)
{
    double s = 0; for (size_t i = a; i < b; ++i) s += (double) x[i] * x[i];
    return (float) std::sqrt (s / (double) jmax<size_t> (1, b - a));
}
static float sineGainDb (VocalStripProcessor& p, float freq, float amp = 0.1f)
{
    const double fs = 48000;
    auto y = run (p, [&] (int i) { return amp * (float) std::sin (MathConstants<double>::twoPi * freq * i / fs); }, 48000);
    return Decibels::gainToDecibels (rms (y, 24000, y.size()) / (amp / std::sqrt (2.0f)));
}
static bool finiteAndBounded (const std::vector<float>& y, float lim)
{
    for (auto v : y) if (! std::isfinite (v) || std::abs (v) > lim) return false;
    return true;
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const double fs = 48000;
    Random rnd (11);
    auto noise = [&] (float a) { return [&rnd, a] (int) { return (rnd.nextFloat() * 2 - 1) * a; }; };

    // ---- all modules off = transparent ----------------------------------
    {
        VocalStripProcessor p; p.prepareToPlay (fs, 512); allOff (p);
        auto y = run (p, [] (int i) { return 0.3f * (float) std::sin (i * 0.031); }, 9600);
        float err = 0; for (int i = 0; i < 9600; ++i) err = jmax (err, std::abs (y[(size_t) i] - 0.3f * (float) std::sin (i * 0.031)));
        check (err < 1e-5f, "all modules off is bit-transparent (err " + String (err, 7) + ")");
    }

    // ---- EQ -------------------------------------------------------------
    {
        VocalStripProcessor p; p.prepareToPlay (fs, 512); allOff (p);
        setP (p, "eq_on", 1); setP (p, "eq1_on", 0); setP (p, "eq4_on", 1); setP (p, "eq4_type", 0);
        setP (p, "eq4_freq", 1000); setP (p, "eq4_gain", 12); setP (p, "eq4_q", 1);
        const float g1k = sineGainDb (p, 1000), g100 = sineGainDb (p, 100);
        std::printf ("  EQ bell +12 @1k: %.2f dB at 1k, %.2f dB at 100 Hz\n", g1k, g100);
        check (std::abs (g1k - 12.0f) < 0.5f && std::abs (g100) < 1.0f, "EQ bell boost is accurate");

        setP (p, "eq4_type", 3); setP (p, "eq4_q", 8);  // notch
        const float n1k = sineGainDb (p, 1000);
        std::printf ("  EQ notch @1k: %.1f dB\n", n1k);
        check (n1k < -20.0f, "EQ notch removes a resonance");

        setP (p, "eq4_on", 0); setP (p, "eq1_on", 1); setP (p, "eq1_type", 4); setP (p, "eq1_freq", 200);
        const float lc50 = sineGainDb (p, 50), lc1k = sineGainDb (p, 2000);
        std::printf ("  EQ low cut 200 Hz: %.1f dB at 50 Hz, %.2f dB at 2k\n", lc50, lc1k);
        check (lc50 < -35.0f && std::abs (lc1k) < 0.5f, "EQ low cut (24 dB/oct)");
    }

    // ---- compressor models ---------------------------------------------
    for (int m = 0; m < 3; ++m)
    {
        VocalStripProcessor p; p.prepareToPlay (fs, 512); allOff (p);
        setP (p, "comp_on", 1); setP (p, "comp_model", (float) m); setP (p, "comp_thresh", -30); setP (p, "comp_ratio", 4); setP (p, "comp_makeup", 0);
        auto y = run (p, [] (int i) { return 0.5f * (float) std::sin (i * 0.06); }, 96000);
        std::printf ("  comp %s: GR %.1f dB\n", m == 0 ? "OPTO" : m == 1 ? "VCA " : "FET ", p.compGr.load());
        check (p.compGr.load() > 8.0f && finiteAndBounded (y, 2.0f), String ("compressor model ") + String (m) + " compresses");
    }

    // ---- de-esser -------------------------------------------------------
    {
        auto level = [&] (float freq)
        {
            VocalStripProcessor p; p.prepareToPlay (fs, 512); allOff (p);
            setP (p, "ds_on", 1); setP (p, "ds_freq", 6000); setP (p, "ds_thresh", -40); setP (p, "ds_range", 12);
            return sineGainDb (p, freq, 0.3f);
        };
        const float s = level (7000), low = level (300);
        std::printf ("  de-esser: 7 kHz %.1f dB, 300 Hz %.2f dB\n", s, low);
        check (s < -6.0f && std::abs (low) < 0.5f, "de-esser reduces only the S band");
    }

    // ---- saturation bands are flat at zero drive -------------------------
    {
        VocalStripProcessor p; p.prepareToPlay (fs, 512); allOff (p);
        setP (p, "sat_on", 1); setP (p, "sat_low", 0); setP (p, "sat_mid", 0); setP (p, "sat_high", 0); setP (p, "sat_air", 0); setP (p, "sat_tone", 0);
        bool flat = true;
        for (float f : { 60.f, 250.f, 1000.f, 4000.f, 9000.f })
        {
            const float g = sineGainDb (p, f);
            std::printf ("  sat crossover @%5.0f Hz: %+.2f dB\n", f, g);
            flat &= std::abs (g) < 0.3f;
        }
        check (flat, "3-band split recombines flat");
        setP (p, "sat_mid", 80); setP (p, "sat_air", 80); setP (p, "sat_tone", 80);
        auto y = run (p, noise (0.7f), 48000);
        check (finiteAndBounded (y, 3.0f), "heavy saturation stays stable");
    }

    // ---- delay timing ---------------------------------------------------
    {
        VocalStripProcessor p; p.prepareToPlay (fs, 512); allOff (p);
        setP (p, "dly_on", 1); setP (p, "dly_sync", 0); setP (p, "dly_time", 250); setP (p, "dly_mode", 0);
        setP (p, "dly_mix", 100); setP (p, "dly_duck", 0); setP (p, "dly_lowcut", 20); setP (p, "dly_highcut", 20000); setP (p, "dly_fb", 0);
        auto y = run (p, [] (int i) { return i == 100 ? 1.0f : 0.0f; }, 24000);
        int idx = 0; float best = 0;
        for (int i = 1000; i < 24000; ++i) if (std::abs (y[(size_t) i]) > best) { best = std::abs (y[(size_t) i]); idx = i; }
        std::printf ("  delay echo at sample %d (expected %d), level %.2f\n", idx, 100 + 12000, best);
        check (std::abs (idx - 12100) <= 2 && best > 0.5f, "delay time is accurate");

        setP (p, "dly_mode", 2); setP (p, "dly_fb", 90);
        auto z = run (p, noise (0.5f), 96000);
        check (finiteAndBounded (z, 4.0f), "ping-pong with high feedback is stable");
    }

    // ---- reverb + glue ---------------------------------------------------
    {
        VocalStripProcessor p; p.prepareToPlay (fs, 512); allOff (p);
        setP (p, "rev_on", 1); setP (p, "rev_mix", 100);
        auto y = run (p, [] (int i) { return i < 4800 ? 0.5f * (float) std::sin (i * 0.05) : 0.0f; }, 48000);
        const float tail = rms (y, 9600, 19200);
        std::printf ("  reverb tail rms after input stops: %.4f\n", tail);
        check (tail > 0.005f && finiteAndBounded (y, 3.0f), "reverb produces a tail");

        allOff (p); setP (p, "glue_on", 1); setP (p, "glue_thresh", -20); setP (p, "glue_ratio", 1);
        run (p, [] (int i) { return 0.5f * (float) std::sin (i * 0.05); }, 48000);
        std::printf ("  glue GR %.1f dB\n", p.glueGr.load());
        check (p.glueGr.load() > 4.0f, "glue compressor works");
    }

    // ---- presets --------------------------------------------------------
    {
        VocalStripProcessor p; p.prepareToPlay (fs, 400);
        bool ok = true;
        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            p.setCurrentProgram (i);
            auto y = run (p, noise (0.4f), 48000, 400);
            const float r = rms (y, 24000, y.size());
            std::printf ("  preset %-16s rms %.3f  (in 0.231)  comp GR %.1f  glue GR %.1f\n", p.getProgramName (i).toRawUTF8(), r, p.compGr.load(), p.glueGr.load());
            ok &= finiteAndBounded (y, 4.0f) && r > 0.03f;
        }
        check (ok, "all presets stable and audible");
    }

    // ---- state ----------------------------------------------------------
    {
        VocalStripProcessor a, b;
        a.setCurrentProgram (2);
        MemoryBlock mb; a.getStateInformation (mb);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        check (b.getCurrentProgram() == 2 && b.value ("comp_model") == 2.0f && b.value ("eq5_gain") == 3.0f, "state save/restore");
    }

    // ---- screenshots of every page --------------------------------------
    if (argc > 1)
    {
        VocalStripProcessor p; p.prepareToPlay (fs, 512);
        p.setCurrentProgram (2);
        auto voice = [&] (int i) { return 0.35f * (float) (std::sin (i * 0.02) * std::sin (i * 0.0007) + 0.3 * std::sin (i * 0.21)) + (rnd.nextFloat() - 0.5f) * 0.05f; };
        run (p, voice, 48000);
        std::unique_ptr<AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (vsui::MainPanel::W, vsui::MainPanel::H);
        for (int page = 0; page < 7; ++page)
        {
            static_cast<VocalStripEditor*> (ed.get())->showPage (page);
            for (int k = 0; k < 8; ++k) { run (p, voice, 4096); MessageManager::getInstance()->runDispatchLoopUntil (40); }
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            File out (String (argv[1]) + "-" + String (page) + ".png"); out.deleteFile();
            FileOutputStream os (out);
            PNGImageFormat().writeImageToStream (img, os);
        }
        check (true, "wrote page screenshots");
    }

    std::printf ("\n%s (%d failures)\n", failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED", failures);
    return failures == 0 ? 0 : 1;
}
