#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class SqeletorProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int kMaxSteps = 8;

    struct Step { int noteNumber = -1; }; // -1 = rest

    SqeletorProcessor();
    ~SqeletorProcessor() override = default;

    //==========================================================================
    // AudioProcessor
    void prepareToPlay (double, int) override {}
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms() override                              { return 1; }
    int  getCurrentProgram() override                           { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return {}; }
    void changeProgramName (int, const juce::String&) override  {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Public APVTS — editor attaches controls here
    juce::AudioProcessorValueTreeState apvts;

    // Sequencer state — audio thread writes, editor reads via atomics
    std::array<Step, kMaxSteps> steps_{};
    std::atomic<int>  stepCount_   { 0 };
    std::atomic<int>  currentStep_ { kMaxSteps }; // kMaxSteps = "none playing"

    // Editor → processor messages (lock-free)
    std::array<Step, kMaxSteps> pendingSteps_{};
    std::atomic<bool> reorderPending_ { false };
    std::atomic<bool> restPending_    { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void sendNoteOff (juce::MidiBuffer& out, int sampleOffset = 0);
    int  resolveStep (int rawStep, int stepCount);
    void regenerateShuffle (int stepCount);

    bool wasRecording_ { false };
    bool wasPlaying_   { false };
    int  lastSentNote_ { -1 };

    // Random/shuffle state
    std::array<int, kMaxSteps> shuffleOrder_{};
    int shuffleStepCount_   { 0 };
    int lastShuffleRawStep_ { -1 };
    juce::Random rng_;

    int lastRawStep_ { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqeletorProcessor)
};
