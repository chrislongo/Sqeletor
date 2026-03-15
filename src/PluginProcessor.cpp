#include "PluginProcessor.h"
#include "PluginEditor.h"

// stepsPerBeat for each rate option:
// 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/4., 1/8., 1/16., 1/4T, 1/8T, 1/16T
static constexpr double kStepsPerBeat[] = {
    0.25,           // 1/1
    0.5,            // 1/2
    1.0,            // 1/4
    2.0,            // 1/8
    4.0,            // 1/16
    8.0,            // 1/32
    1.0 / 1.5,      // 1/4.  (dotted quarter = 1.5 beats)
    2.0 / 1.5,      // 1/8.  (dotted eighth  = 0.75 beats)
    4.0 / 1.5,      // 1/16. (dotted 16th    = 0.375 beats)
    1.5,            // 1/4T  (quarter triplet = 2/3 beat)
    3.0,            // 1/8T  (eighth triplet  = 1/3 beat)
    6.0,            // 1/16T (16th triplet    = 1/6 beat)
};

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
SqeletorProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rate", 1 }, "Rate",
        juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                            "1/4.", "1/8.", "1/16.", "1/4T", "1/8T", "1/16T" },
        3)); // default: 1/8

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "Forward", "Reverse", "Random" },
        0));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "recording", 1 }, "Recording", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "locked", 1 }, "Locked", false));

    return layout;
}

SqeletorProcessor::SqeletorProcessor()
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "State", createParameterLayout())
{
    for (int i = 0; i < kMaxSteps; ++i)
        shuffleOrder_[static_cast<size_t> (i)] = i;
}

//==============================================================================
void SqeletorProcessor::releaseResources()
{
    juce::MidiBuffer dummy;
    sendNoteOff (dummy);
}

void SqeletorProcessor::sendNoteOff (juce::MidiBuffer& out, int sampleOffset)
{
    if (lastSentNote_ >= 0)
    {
        out.addEvent (juce::MidiMessage::noteOff (1, lastSentNote_), sampleOffset);
        lastSentNote_ = -1;
    }
}

void SqeletorProcessor::regenerateShuffle (int stepCount)
{
    for (int i = 0; i < stepCount; ++i)
        shuffleOrder_[static_cast<size_t> (i)] = i;

    for (int i = stepCount - 1; i > 0; --i)
    {
        int j = static_cast<int> (rng_.nextInt (i + 1));
        std::swap (shuffleOrder_[static_cast<size_t> (i)],
                   shuffleOrder_[static_cast<size_t> (j)]);
    }

    shuffleStepCount_ = stepCount;
}

int SqeletorProcessor::resolveStep (int rawStep, int stepCount)
{
    auto* modeParam = static_cast<juce::AudioParameterChoice*> (apvts.getParameter ("mode"));
    int mode = modeParam->getIndex();

    if (mode == 1) // Reverse
        return stepCount - 1 - rawStep;

    if (mode == 2) // Random/shuffle
    {
        if (shuffleStepCount_ != stepCount)
            regenerateShuffle (stepCount);
        else if (rawStep == 0 && lastShuffleRawStep_ > 0)
            regenerateShuffle (stepCount);

        lastShuffleRawStep_ = rawStep;
        return shuffleOrder_[static_cast<size_t> (rawStep)];
    }

    return rawStep; // Forward
}

//==============================================================================
void SqeletorProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi)
{
    buffer.clear();
    juce::MidiBuffer outputMidi;
    outputMidi.addEvents (midi, 0, -1, 0); // always pass through incoming MIDI

    // Apply pending tile reorder from editor drag
    if (reorderPending_.exchange (false))
        steps_ = pendingSteps_;

    auto* recParam = static_cast<juce::AudioParameterBool*> (apvts.getParameter ("recording"));
    bool isRecording = recParam->get();

    // Detect recording start — clear sequence and silence any playing note
    if (isRecording && ! wasRecording_)
    {
        sendNoteOff (outputMidi);
        for (auto& s : steps_) s.noteNumber = -1;
        stepCount_.store (0);
        currentStep_.store (kMaxSteps);
        lastShuffleRawStep_ = -1;
    }
    wasRecording_ = isRecording;

    // Apply pending rest (set by editor when Rest tile is clicked during recording)
    if (restPending_.exchange (false) && isRecording)
    {
        int count = stepCount_.load();
        if (count < kMaxSteps)
        {
            steps_[static_cast<size_t> (count)].noteNumber = -1; // rest
            stepCount_.store (count + 1);
            if (count + 1 >= kMaxSteps)
                recParam->setValueNotifyingHost (0.0f);
        }
    }

    // Recording mode: consume incoming note-ons, emit nothing.
    // Deliberately placed before any playhead check — recording works regardless
    // of transport state.
    if (isRecording)
    {
        int count = stepCount_.load();
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn() && count < kMaxSteps)
            {
                steps_[static_cast<size_t> (count)].noteNumber = msg.getNoteNumber();
                stepCount_.store (++count);
                if (count >= kMaxSteps)
                {
                    recParam->setValueNotifyingHost (0.0f);
                    break;
                }
            }
        }
        midi.swapWith (outputMidi);
        return;
    }

    // Playback requires a valid playhead
    auto* playHead = getPlayHead();
    if (playHead == nullptr) { midi.swapWith (outputMidi); return; }

    auto posOpt = playHead->getPosition();
    if (! posOpt.hasValue()) { midi.swapWith (outputMidi); return; }

    auto& pos = *posOpt;

    int stepCount = stepCount_.load();
    if (stepCount == 0) { midi.swapWith (outputMidi); return; }

    bool isPlaying = pos.getIsPlaying();

    if (! isPlaying)
    {
        sendNoteOff (outputMidi);
        currentStep_.store (kMaxSteps);
        lastShuffleRawStep_ = -1;
        wasPlaying_ = false;
        midi.swapWith (outputMidi);
        return;
    }

    auto ppqOpt = pos.getPpqPosition();
    if (! ppqOpt.hasValue()) { midi.swapWith (outputMidi); return; }

    double ppq = juce::jmax (0.0, *ppqOpt);

    auto* rateParam = static_cast<juce::AudioParameterChoice*> (apvts.getParameter ("rate"));
    double stepsPerBeat = kStepsPerBeat[rateParam->getIndex()];

    double beatInLoop = std::fmod (ppq * stepsPerBeat, static_cast<double> (stepCount));
    if (beatInLoop < 0.0) beatInLoop += stepCount;

    int rawStep  = juce::jlimit (0, stepCount - 1, static_cast<int> (beatInLoop));
    int playStep = resolveStep (rawStep, stepCount);

    if (playStep != currentStep_.load())
    {
        sendNoteOff (outputMidi);
        currentStep_.store (playStep);

        int note = steps_[static_cast<size_t> (playStep)].noteNumber;
        if (note >= 0)
        {
            outputMidi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            lastSentNote_ = note;
        }
    }

    wasPlaying_ = true;
    midi.swapWith (outputMidi);
}

//==============================================================================
void SqeletorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, true);

    stream.writeString (apvts.copyState().toXmlString());

    int count = stepCount_.load();
    stream.writeInt (count);
    for (int i = 0; i < count; ++i)
        stream.writeInt (steps_[static_cast<size_t> (i)].noteNumber);
}

void SqeletorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

    auto xml = stream.readString();
    if (auto vt = juce::ValueTree::fromXml (xml); vt.isValid())
        apvts.replaceState (vt);

    int count = juce::jlimit (0, kMaxSteps, stream.readInt());
    for (int i = 0; i < count; ++i)
        steps_[static_cast<size_t> (i)].noteNumber = stream.readInt();
    stepCount_.store (count);
    currentStep_.store (kMaxSteps);
}

//==============================================================================
juce::AudioProcessorEditor* SqeletorProcessor::createEditor()
{
    return new SqeletorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SqeletorProcessor();
}
