#include "PluginProcessor.h"
#include "PluginEditor.h"

// Scale intervals (semitones from root) — index matches scaleIdx-1
// Order: Major, Minor, Dorian, Phrygian, Lydian, Mixolydian, Locrian
static constexpr int kScaleIntervals[7][7] = {
    { 0, 2, 4, 5, 7, 9, 11 },   // Major (Ionian)
    { 0, 2, 3, 5, 7, 8, 10 },   // Minor (Aeolian)
    { 0, 2, 3, 5, 7, 9, 10 },   // Dorian
    { 0, 1, 3, 5, 7, 8, 10 },   // Phrygian
    { 0, 2, 4, 6, 7, 9, 11 },   // Lydian
    { 0, 2, 4, 5, 7, 9, 10 },   // Mixolydian
    { 0, 1, 3, 5, 6, 8, 10 },   // Locrian
};

int FreakQuencerProcessor::snapToScale (int noteNum, int key, int scaleIdx)
{
    if (scaleIdx == 0 || noteNum < 0) return noteNum;  // Off or rest

    const auto* intervals = kScaleIntervals[scaleIdx - 1];
    int pitchClass = noteNum % 12;
    int octave     = noteNum / 12;

    // Find pitch class in the scale with the smallest circular distance
    int bestPc   = pitchClass;
    int bestDist = 13;
    for (int i = 0; i < 7; ++i)
    {
        int scalePc = (key + intervals[i]) % 12;
        int d = std::abs (pitchClass - scalePc);
        if (d > 6) d = 12 - d;   // circular distance
        if (d < bestDist) { bestDist = d; bestPc = scalePc; }
    }

    if (bestDist == 0) return noteNum;   // already in scale

    // Try the same bestPc in the adjacent octaves and pick the nearest MIDI note
    int snapped    = noteNum;
    int minAbsDist = 127;
    for (int oct = octave - 1; oct <= octave + 1; ++oct)
    {
        int candidate = oct * 12 + bestPc;
        if (candidate < 0 || candidate > 127) continue;
        int d = std::abs (candidate - noteNum);
        if (d < minAbsDist) { minAbsDist = d; snapped = candidate; }
    }
    return snapped;
}

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
FreakQuencerProcessor::createParameterLayout()
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

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "key", 1 }, "Key",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F",
                            "F#", "G", "G#", "A", "A#", "B" },
        0));  // default: C

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "scale", 1 }, "Scale",
        juce::StringArray { "Off", "Major", "Minor",
                            "Dorian", "Phrygian", "Lydian",
                            "Mixolyd.", "Locrian" },
        0));  // default: Off

    return layout;
}

FreakQuencerProcessor::FreakQuencerProcessor()
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "State", createParameterLayout())
{
    for (int i = 0; i < kMaxSteps; ++i)
    {
        shuffleOrder_[static_cast<size_t> (i)] = i;
        steps_[static_cast<size_t> (i)].noteNumber = -1;  // rest
    }
    stepCount_.store (kMaxSteps);
}

//==============================================================================
void FreakQuencerProcessor::releaseResources()
{
    juce::MidiBuffer dummy;
    sendNoteOff (dummy);
}

void FreakQuencerProcessor::sendNoteOff (juce::MidiBuffer& out, int sampleOffset)
{
    if (lastSentNote_ >= 0)
    {
        out.addEvent (juce::MidiMessage::noteOff (1, lastSentNote_), sampleOffset);
        lastSentNote_ = -1;
    }
}

void FreakQuencerProcessor::regenerateShuffle (int stepCount)
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

int FreakQuencerProcessor::resolveStep (int rawStep, int stepCount)
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
void FreakQuencerProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi)
{
    buffer.clear();
    juce::MidiBuffer outputMidi;
    outputMidi.addEvents (midi, 0, -1, 0); // always pass through incoming MIDI

    // Apply pending tile reorder from editor drag
    if (reorderPending_.exchange (false))
        steps_ = pendingSteps_;

    // Apply pending note edit from editor drag / toggle
    if (int s = pendingNoteEditStep_.exchange (-1); s >= 0 && s < kMaxSteps)
    {
        auto& step  = steps_[static_cast<size_t> (s)];
        int newNote = pendingNoteEditNote_.load();
        // When switching to rest, preserve the last real note for later restore
        if (newNote < 0 && step.noteNumber >= 0)
            step.savedNote = step.noteNumber;
        step.noteNumber = newNote;
    }

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
        lastRawStep_ = -1;
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
        if (rawStep != lastRawStep_)
        {
            // Genuine step boundary — fire the note
            sendNoteOff (outputMidi);
            currentStep_.store (playStep);

            int note = steps_[static_cast<size_t> (playStep)].noteNumber;
            if (note >= 0)
            {
                auto* keyParam   = static_cast<juce::AudioParameterChoice*> (apvts.getParameter ("key"));
                auto* scaleParam = static_cast<juce::AudioParameterChoice*> (apvts.getParameter ("scale"));
                note = snapToScale (note, keyParam->getIndex(), scaleParam->getIndex());
                outputMidi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
                lastSentNote_ = note;
            }
        }
        else
        {
            // Mode changed mid-step — update tracking silently, no MIDI
            currentStep_.store (playStep);
        }
    }

    lastRawStep_ = rawStep;

    wasPlaying_ = true;
    midi.swapWith (outputMidi);
}

//==============================================================================
void FreakQuencerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, true);

    stream.writeString (apvts.copyState().toXmlString());

    int count = stepCount_.load();
    stream.writeInt (count);
    for (int i = 0; i < count; ++i)
        stream.writeInt (steps_[static_cast<size_t> (i)].noteNumber);
}

void FreakQuencerProcessor::setStateInformation (const void* data, int sizeInBytes)
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
juce::AudioProcessorEditor* FreakQuencerProcessor::createEditor()
{
    return new FreakQuencerEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreakQuencerProcessor();
}
