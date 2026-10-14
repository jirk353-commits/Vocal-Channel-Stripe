#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <unordered_map>
#include "Modules.h"

class VocalStripProcessor : public juce::AudioProcessor
{
public:
    VocalStripProcessor();
    ~VocalStripProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Vocal Channel Strip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String eqId (int band, const char* what) { return "eq" + juce::String (band + 1) + "_" + what; }
    static const juce::StringArray& delayDivisions();

    float value (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float v);
    void applyReverbEnvironment (int type);

    // metering
    std::atomic<float> inLevel { -100 }, outLevel { -100 }, compGr { 0 }, deessGr { 0 }, glueGr { 0 };
    std::atomic<double> hostBpm { 120.0 };

    // analyzer feed (post-EQ, mono)
    juce::AbstractFifo analyzerFifo { 8192 };
    std::vector<float> analyzerData = std::vector<float> (8192, 0.0f);
    double getSampleRateSafe() const { return sr; }

private:
    float p (const char* id) const { return cache.at (id)->load(); }

    vs::Equalizer eq;
    vs::Compressor comp;
    vs::DeEsser deess;
    vs::Saturator sat;
    vs::Delay delay;
    gv::Reverb reverb;
    vs::Biquad2 revLow, revHigh;
    vs::Glue glue;

    juce::SmoothedValue<float> inG, outG, dlyMix, revMix;
    float lastRevLow = -1, lastRevHigh = -1;
    double sr = 48000.0;
    int currentProgram = 0;
    std::unordered_map<std::string, std::atomic<float>*> cache;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalStripProcessor)
};
