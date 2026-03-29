#include "PluginEditor.h"

namespace {
    constexpr int kPadding      = 14;
    constexpr int kGap          = 8;
    constexpr int kCtrlColWidth = 68;
    constexpr int kTileW        = 120;
    constexpr int kTileH        = 120;
    constexpr int kGridCols     = 4;
    constexpr int kGridRows     = 2;
    constexpr int kNumCtrl      = 4;
    constexpr int kNumScaleCtrl = 2;   // Key + Scale on right

    constexpr int kGridW  = kGridCols * kTileW + (kGridCols - 1) * kGap;
    constexpr int kPanelW = kPadding * 2 + kCtrlColWidth + kGap + kGridW + kGap + kCtrlColWidth;
    constexpr int kPanelH = kPadding * 2 + kGridRows * kTileH + (kGridRows - 1) * kGap;

    constexpr int kCtrlTileH  = (kPanelH - kPadding * 2 - (kNumCtrl      - 1) * kGap) / kNumCtrl;
    constexpr int kScaleTileH = (kPanelH - kPadding * 2 - (kNumScaleCtrl - 1) * kGap) / kNumScaleCtrl;

    // X origin of the right (scale) control column
    constexpr int kRightColX = kPadding + kCtrlColWidth + kGap + kGridW + kGap;
}

//==============================================================================
FreakQuencerEditor::FreakQuencerEditor (FreakQuencerProcessor& p)
    : AudioProcessorEditor (p), proc_ (p)
{
    setSize (kPanelW, kPanelH);
    startTimerHz (60);
}

void FreakQuencerEditor::timerCallback()
{
    int step  = proc_.currentStep_.load();
    int count = proc_.stepCount_.load();
    if (step != lastDisplayedStep_ || count != lastDisplayedCount_)
    {
        if (step != lastDisplayedStep_)
            stepChangeTime_ = juce::Time::getMillisecondCounter();
        lastDisplayedStep_  = step;
        lastDisplayedCount_ = count;
        repaint();
    }

    // Keep repainting during the flash decay window (~150ms)
    auto elapsed = juce::Time::getMillisecondCounter() - stepChangeTime_;
    if (elapsed < 150)
        repaint();
}

//==============================================================================
// Layout helpers

juce::Rectangle<int> FreakQuencerEditor::getCtrlTileBounds (int index) const
{
    return { kPadding,
             kPadding + index * (kCtrlTileH + kGap),
             kCtrlColWidth,
             kCtrlTileH };
}

juce::Rectangle<int> FreakQuencerEditor::getNoteTileBounds (int index) const
{
    int col = index % kGridCols;
    int row = index / kGridCols;
    return { kPadding + kCtrlColWidth + kGap + col * (kTileW + kGap),
             kPadding + row * (kTileH + kGap),
             kTileW, kTileH };
}

juce::Rectangle<int> FreakQuencerEditor::getScaleTileBounds (int index) const
{
    return { kRightColX,
             kPadding + index * (kScaleTileH + kGap),
             kCtrlColWidth,
             kScaleTileH };
}

int FreakQuencerEditor::noteTileAt (juce::Point<int> pos) const
{
    for (int i = 0; i < FreakQuencerProcessor::kMaxSteps; ++i)
        if (getNoteTileBounds (i).contains (pos))
            return i;
    return -1;
}

//==============================================================================
// Drawing

void FreakQuencerEditor::drawCtrlTile (juce::Graphics& g,
                                    juce::Rectangle<int> bounds,
                                    const juce::String& /*label*/,
                                    const juce::String& value,
                                    bool active) const
{
    g.setColour (active ? juce::Colour (0xffe8347a) : juce::Colour (0xff242432));
    g.fillRoundedRectangle (bounds.toFloat(), 4.0f);

    g.setColour (active ? juce::Colours::white : juce::Colour (0xffddddee));
    float fontSize = (value.length() > 7) ? 10.0f
                   : (value.length() > 4) ? 12.0f
                   : (value.length() > 3) ? 14.0f : 18.0f;
    g.setFont (juce::Font (juce::FontOptions().withHeight (fontSize)));
    g.drawText (value, bounds.toFloat(), juce::Justification::centred);
}


namespace {
    // 12-colour palette — one per pitch class (C through B), Vital-style neons
    static const juce::Colour kNoteColours[12] = {
        juce::Colour (0xffe8347a),   //  0 C  hot pink
        juce::Colour (0xffcc3aaa),   //  1 C# rose-magenta
        juce::Colour (0xffb43de8),   //  2 D  violet
        juce::Colour (0xff6b3de8),   //  3 D# indigo
        juce::Colour (0xff3d7de8),   //  4 E  blue
        juce::Colour (0xff35c4e8),   //  5 F  sky
        juce::Colour (0xff35e8c4),   //  6 F# teal
        juce::Colour (0xff35e878),   //  7 G  green
        juce::Colour (0xffa8e835),   //  8 G# lime
        juce::Colour (0xffe8d435),   //  9 A  yellow
        juce::Colour (0xffe87835),   // 10 A# orange
        juce::Colour (0xffe83535),   // 11 B  red
    };
}

void FreakQuencerEditor::drawNoteTile (juce::Graphics& g,
                                    juce::Rectangle<int> bounds,
                                    const juce::String& noteName,
                                    bool isActive,
                                    bool isEmpty,
                                    bool isRest,
                                    juce::Colour tileColour,
                                    float flashBrightness,
                                    int octave) const
{
    if (isEmpty)
    {
        g.setColour (juce::Colour (0x33aaaacc));
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 4.0f, 1.0f);
        return;
    }

    if (isActive)
    {
        auto fillColour = tileColour.interpolatedWith (juce::Colours::white, flashBrightness);
        g.setColour (fillColour);
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff242432));
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
    }

    auto textColour = isActive ? juce::Colours::white
                               : (isRest ? tileColour.withAlpha (0.35f) : tileColour);
    g.setColour (textColour);

    float fontSize = (noteName.length() > 1) ? 68.0f : 96.0f;
    g.setFont (juce::Font (juce::FontOptions().withHeight (fontSize).withStyle ("Bold")));
    g.drawText (noteName, bounds, juce::Justification::centred);

    // Octave label — bottom-right corner, small
    if (octave != -100 && ! isRest)
    {
        auto octColour = isActive ? juce::Colours::white.withAlpha (0.55f)
                                  : textColour.withAlpha (0.45f);
        g.setColour (octColour);
        g.setFont (juce::Font (juce::FontOptions().withHeight (16.0f).withStyle ("Bold")));
        g.drawText (juce::String (octave),
                    bounds.reduced (6).removeFromBottom (22).removeFromRight (28),
                    juce::Justification::centredRight);
    }
}

//==============================================================================
void FreakQuencerEditor::paint (juce::Graphics& g)
{
    // Dark panel background
    g.fillAll (juce::Colour (0xff181820));
    juce::ColourGradient overlay (juce::Colour (0x10ffffff), 0.0f, 0.0f,
                                   juce::Colour (0x10000000), 0.0f, (float) getHeight(), false);
    g.setGradientFill (overlay);
    g.fillAll();

    auto* recParam   = static_cast<juce::AudioParameterBool*>   (proc_.apvts.getParameter ("recording"));
    auto* modeParam  = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("mode"));
    auto* rateParam  = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("rate"));
    auto* keyParam   = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("key"));
    auto* scaleParam = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("scale"));
    bool isRecording = recParam->get();
    bool scaleActive = scaleParam->getIndex() != 0;

    static const juce::String kModeSymbols[] = {
        juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x92")),   // →
        juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x90")),   // ←
        juce::String (juce::CharPointer_UTF8 ("\xe2\x87\x84"))    // ⇄
    };

    // ── Control tiles ──────────────────────────────────────────────────────
    drawCtrlTile (g, getCtrlTileBounds (0), "RATE", rateParam->getCurrentChoiceName(), false);
    drawCtrlTile (g, getCtrlTileBounds (1), "MODE", kModeSymbols[modeParam->getIndex()], true);
    drawCtrlTile (g, getCtrlTileBounds (2), "REC",
                  juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x8f")), isRecording); // ●
    drawCtrlTile (g, getCtrlTileBounds (3), "REST",
                  juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94")), false);        // —

    // ── Scale/Key tiles (right column) ─────────────────────────────────────
    drawCtrlTile (g, getScaleTileBounds (0), "KEY",   keyParam->getCurrentChoiceName(),   scaleActive);
    drawCtrlTile (g, getScaleTileBounds (1), "SCALE", scaleParam->getCurrentChoiceName(), scaleActive);

    // ── Note tiles ─────────────────────────────────────────────────────────
    int stepCount  = proc_.stepCount_.load();
    int activeStep = proc_.currentStep_.load();

    // LED flash: bright white burst on beat, decays to tile colour over 150ms
    auto elapsed = (float) (juce::Time::getMillisecondCounter() - stepChangeTime_);
    float flashBrightness = (elapsed < 20.0f) ? 1.0f
                          : (elapsed < 150.0f) ? 1.0f - (elapsed - 20.0f) / 130.0f
                          : 0.0f;

    for (int slot = 0; slot < FreakQuencerProcessor::kMaxSteps; ++slot)
    {
        auto bounds  = getNoteTileBounds (slot);
        bool isEmpty = slot >= stepCount;

        if (isEmpty)
        {
            drawNoteTile (g, bounds, {}, false, true, false);
            continue;
        }

        bool isActive = (slot == activeStep);

        // For in-progress drag, override displayed note
        int noteNum;
        if (dragTile_ == slot)
            noteNum = dragCurrentNote_;
        else
            noteNum = proc_.steps_[static_cast<size_t> (slot)].noteNumber;

        // Apply scale snap for display (matches what will play)
        int displayNote = FreakQuencerProcessor::snapToScale (
                              noteNum, keyParam->getIndex(), scaleParam->getIndex());

        bool isRest = (displayNote < 0);
        juce::String noteName = isRest
            ? juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93")) // –
            : juce::MidiMessage::getMidiNoteName (displayNote, true, false, 4);
        auto colour = isRest ? juce::Colour (0xff555566)
                             : kNoteColours[static_cast<size_t> (displayNote) % 12];
        int octave = isRest ? -100 : (displayNote / 12 - 1);

        drawNoteTile (g, bounds, noteName, isActive, false, isRest, colour,
                      isActive ? flashBrightness : 0.0f, octave);
    }
}

//==============================================================================
void FreakQuencerEditor::mouseDown (const juce::MouseEvent& event)
{
    auto pos = event.getPosition();

    // ── Control tiles ────────────────────────────────────────────────────
    for (int i = 0; i < kNumCtrl; ++i)
    {
        if (! getCtrlTileBounds (i).contains (pos)) continue;

        switch (i)
        {
            case 0: // RATE — start drag-to-cycle
            {
                auto* p = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("rate"));
                ctrlDragIndex_    = 0;
                ctrlDragStartY_   = pos.y;
                ctrlDragBaseStep_ = p->getIndex();
                return;
            }
            case 1: // MODE — start drag-to-cycle
            {
                auto* p = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("mode"));
                ctrlDragIndex_    = 1;
                ctrlDragStartY_   = pos.y;
                ctrlDragBaseStep_ = p->getIndex();
                return;
            }
            case 2: // REC — toggle
            {
                auto* p = static_cast<juce::AudioParameterBool*> (proc_.apvts.getParameter ("recording"));
                p->setValueNotifyingHost (p->get() ? 0.0f : 1.0f);
                break;
            }
            case 3: // REST — fire if currently recording
            {
                auto* recP = static_cast<juce::AudioParameterBool*> (proc_.apvts.getParameter ("recording"));
                if (recP->get())
                    proc_.restPending_.store (true);
                break;
            }
        }
        repaint();
        return;
    }

    // ── Scale / Key tiles (right column) ─────────────────────────────────
    for (int i = 0; i < kNumScaleCtrl; ++i)
    {
        if (! getScaleTileBounds (i).contains (pos)) continue;
        const char* paramId = (i == 0) ? "key" : "scale";
        auto* p = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter (paramId));
        ctrlDragIndex_    = 4 + i;
        ctrlDragStartY_   = pos.y;
        ctrlDragBaseStep_ = p->getIndex();
        return;
    }

    // ── Note tiles ───────────────────────────────────────────────────────
    auto* recParam = static_cast<juce::AudioParameterBool*> (proc_.apvts.getParameter ("recording"));
    if (recParam->get()) return;

    int tileIndex = noteTileAt (pos);
    if (tileIndex < 0 || tileIndex >= proc_.stepCount_.load()) return;

    int currentNote = proc_.steps_[static_cast<size_t> (tileIndex)].noteNumber;
    bool isRest = (currentNote < 0);

    // Cmd+click: toggle rest (restores previous note when un-resting)
    if (event.mods.isCommandDown())
    {
        int restoreNote = isRest ? proc_.steps_[static_cast<size_t> (tileIndex)].savedNote : -1;
        proc_.pendingNoteEditNote_.store (restoreNote);
        proc_.pendingNoteEditStep_.store (tileIndex);
        repaint();
        return;
    }

    dragTile_        = tileIndex;
    dragStartX_      = pos.x;
    dragStartY_      = pos.y;
    dragCurrentNote_ = currentNote;
    dragBaseIdx_     = isRest ? 0 : (currentNote % 12);
    dragBaseOctave_  = isRest ? 4 : (currentNote / 12 - 1);
    repaint();
}

void FreakQuencerEditor::mouseDrag (const juce::MouseEvent& event)
{
    // ── Ctrl tile drag (RATE / MODE / KEY / SCALE) ──────────────────────
    if (ctrlDragIndex_ >= 0)
    {
        const char* paramId = (ctrlDragIndex_ == 0) ? "rate"
                            : (ctrlDragIndex_ == 1) ? "mode"
                            : (ctrlDragIndex_ == 4) ? "key"
                            :                         "scale";
        auto* p = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter (paramId));
        int numChoices = (int) p->choices.size();
        int deltaSteps = (ctrlDragStartY_ - event.getPosition().y) / 32;
        int newIndex   = ((ctrlDragBaseStep_ + deltaSteps) % numChoices + numChoices) % numChoices;
        if (newIndex != p->getIndex())
        {
            float norm = static_cast<float> (newIndex) / static_cast<float> (numChoices - 1);
            p->setValueNotifyingHost (norm);
            repaint();
        }
        return;
    }

    // ── Note tile drag ───────────────────────────────────────────────────
    if (dragTile_ < 0) return;

    // Left/right → pitch class (wraps through 12 chromatic positions, no rest)
    int deltaX    = event.getPosition().x - dragStartX_;
    int newIdx    = ((dragBaseIdx_ + deltaX / 20) % 12 + 12) % 12;

    // Up/down → octave (no wrap, clamp)
    int deltaY    = dragStartY_ - event.getPosition().y;
    int newOctave = juce::jlimit (0, 9, dragBaseOctave_ + deltaY / 40);

    int newNote   = juce::jlimit (0, 127, (newOctave + 1) * 12 + newIdx);

    if (newNote != dragCurrentNote_)
    {
        dragCurrentNote_ = newNote;
        repaint();
    }
}

void FreakQuencerEditor::mouseUp (const juce::MouseEvent& event)
{
    // ── Ctrl tile drag release (RATE / MODE / KEY / SCALE) ──────────────
    if (ctrlDragIndex_ >= 0)
    {
        if (std::abs (event.getPosition().y - ctrlDragStartY_) < 5)
        {
            const char* paramId = (ctrlDragIndex_ == 0) ? "rate"
                                : (ctrlDragIndex_ == 1) ? "mode"
                                : (ctrlDragIndex_ == 4) ? "key"
                                :                         "scale";
            auto* p = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter (paramId));
            int numChoices = (int) p->choices.size();
            int next = (p->getIndex() + 1) % numChoices;
            float norm = static_cast<float> (next) / static_cast<float> (numChoices - 1);
            p->setValueNotifyingHost (norm);
        }
        ctrlDragIndex_ = -1;
        repaint();
        return;
    }

    // ── Note tile drag commit ────────────────────────────────────────────
    if (dragTile_ >= 0)
    {
        proc_.pendingNoteEditNote_.store (dragCurrentNote_);
        proc_.pendingNoteEditStep_.store (dragTile_);

        dragTile_ = -1;
        repaint();
    }
}
