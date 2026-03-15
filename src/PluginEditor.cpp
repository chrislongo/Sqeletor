#include "PluginEditor.h"

namespace {
    constexpr int kPadding      = 14;
    constexpr int kGap          = 8;
    constexpr int kCtrlColWidth = 68;
    constexpr int kTileW        = 120;
    constexpr int kTileH        = 120;
    constexpr int kGridCols     = 4;
    constexpr int kGridRows     = 2;
    constexpr int kNumCtrl      = 5;

    constexpr int kPanelW = kPadding * 2 + kCtrlColWidth + kGap
                            + kGridCols * kTileW + (kGridCols - 1) * kGap;
    constexpr int kPanelH = kPadding * 2 + kGridRows * kTileH + (kGridRows - 1) * kGap;
    constexpr int kCtrlTileH = (kPanelH - kPadding * 2 - (kNumCtrl - 1) * kGap) / kNumCtrl;
}

//==============================================================================
SqeletorEditor::SqeletorEditor (SqeletorProcessor& p)
    : AudioProcessorEditor (p), proc_ (p)
{
    setSize (kPanelW, kPanelH);
    startTimerHz (60);
}

void SqeletorEditor::timerCallback()
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

juce::Rectangle<int> SqeletorEditor::getCtrlTileBounds (int index) const
{
    return { kPadding,
             kPadding + index * (kCtrlTileH + kGap),
             kCtrlColWidth,
             kCtrlTileH };
}

juce::Rectangle<int> SqeletorEditor::getNoteTileBounds (int index) const
{
    int col = index % kGridCols;
    int row = index / kGridCols;
    return { kPadding + kCtrlColWidth + kGap + col * (kTileW + kGap),
             kPadding + row * (kTileH + kGap),
             kTileW, kTileH };
}

int SqeletorEditor::noteTileAt (juce::Point<int> pos) const
{
    for (int i = 0; i < SqeletorProcessor::kMaxSteps; ++i)
        if (getNoteTileBounds (i).contains (pos))
            return i;
    return -1;
}

//==============================================================================
// Drawing

void SqeletorEditor::drawCtrlTile (juce::Graphics& g,
                                    juce::Rectangle<int> bounds,
                                    const juce::String& label,
                                    const juce::String& value,
                                    bool active) const
{
    // Active = hot pink; inactive = dark tile
    g.setColour (active ? juce::Colour (0xffe8347a) : juce::Colour (0xff242432));
    g.fillRoundedRectangle (bounds.toFloat(), 4.0f);

    // Tiny label
    g.setColour (active ? juce::Colour (0xccffffff) : juce::Colour (0x99aaaacc));
    g.setFont (juce::Font (juce::FontOptions().withHeight (7.0f)));
    g.drawText (label, bounds.withHeight (14).translated (0, 4), juce::Justification::centred);

    // Value
    g.setColour (active ? juce::Colours::white : juce::Colour (0xffddddee));
    float fontSize = (value.length() > 4) ? 12.0f : (value.length() > 3) ? 14.0f : 18.0f;
    g.setFont (juce::Font (juce::FontOptions().withHeight (fontSize)));
    g.drawText (value, bounds.withTrimmedTop (10), juce::Justification::centred);
}

void SqeletorEditor::drawLockIcon (juce::Graphics& g,
                                    juce::Rectangle<float> b,
                                    juce::Colour colour,
                                    bool locked) const
{
    // Scale everything to a small icon centred in b
    float iconH   = b.getHeight() * 0.62f;
    float iconW   = iconH * 0.75f;
    float cx      = b.getCentreX();
    float cy      = b.getCentreY() + iconH * 0.05f;

    float bodyH   = iconH * 0.55f;
    float bodyW   = iconW;
    float bodyTop = cy - bodyH * 0.5f + iconH * 0.1f;

    // Body — filled rounded rect
    juce::Rectangle<float> body { cx - bodyW * 0.5f, bodyTop, bodyW, bodyH };
    g.setColour (colour);
    g.fillRoundedRectangle (body, bodyW * 0.12f);

    // Shackle — U-arch above body, stroked
    float sw      = bodyW * 0.58f;          // inner gap width
    float stroke  = bodyW * 0.18f;          // shackle thickness
    float archR   = (sw + stroke) * 0.5f;   // outer radius
    float archTop = bodyTop - archR * 1.1f;

    juce::Path shackle;
    if (locked)
    {
        // Closed: full semicircle, legs drop into the body
        shackle.addArc (cx - archR, archTop, archR * 2.0f, archR * 2.0f,
                        juce::MathConstants<float>::pi,
                        juce::MathConstants<float>::twoPi, true);
        shackle.lineTo (cx + archR, bodyTop + stroke);
        shackle.startNewSubPath (cx - archR, bodyTop + stroke);
        shackle.lineTo (cx - archR, archTop + archR);
    }
    else
    {
        // Open: right leg lifted clear of body
        float legY = archTop - archR * 0.35f;
        shackle.addArc (cx - archR, archTop, archR * 2.0f, archR * 2.0f,
                        juce::MathConstants<float>::pi,
                        juce::MathConstants<float>::twoPi, true);
        shackle.lineTo (cx + archR, legY);
        shackle.startNewSubPath (cx - archR, bodyTop + stroke);
        shackle.lineTo (cx - archR, archTop + archR);
    }

    g.strokePath (shackle, juce::PathStrokeType (stroke,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
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

void SqeletorEditor::drawNoteTile (juce::Graphics& g,
                                    juce::Rectangle<int> bounds,
                                    const juce::String& noteName,
                                    bool isActive,
                                    bool isEmpty,
                                    bool isRest,
                                    juce::Colour tileColour,
                                    float flashBrightness) const
{
    if (isEmpty)
    {
        g.setColour (juce::Colour (0x33aaaacc));
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 4.0f, 1.0f);
        return;
    }

    if (isActive)
    {
        // Flash: interpolate from tile colour toward white on beat
        auto fillColour = tileColour.interpolatedWith (juce::Colours::white, flashBrightness);
        g.setColour (fillColour);
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
    }
    else
    {
        // Inactive: dark fill
        g.setColour (juce::Colour (0xff242432));
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
    }

    auto textColour = isActive ? juce::Colours::white
                               : (isRest ? tileColour.withAlpha (0.35f) : tileColour);
    g.setColour (textColour);

    float fontSize = (noteName.length() > 1) ? 62.0f : 88.0f;
    g.setFont (juce::Font (juce::FontOptions().withHeight (fontSize)));
    g.drawText (noteName, bounds, juce::Justification::centred);
}

//==============================================================================
void SqeletorEditor::paint (juce::Graphics& g)
{
    // Dark panel background
    g.fillAll (juce::Colour (0xff181820));
    juce::ColourGradient overlay (juce::Colour (0x10ffffff), 0.0f, 0.0f,
                                   juce::Colour (0x10000000), 0.0f, (float) getHeight(), false);
    g.setGradientFill (overlay);
    g.fillAll();

    auto* recParam  = static_cast<juce::AudioParameterBool*>   (proc_.apvts.getParameter ("recording"));
    auto* modeParam = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("mode"));
    auto* rateParam = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("rate"));
    auto* lockParam = static_cast<juce::AudioParameterBool*>   (proc_.apvts.getParameter ("locked"));

    bool isRecording = recParam->get();
    bool isLocked    = lockParam->get();

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

    // Lock tile — draw manually to get the icon
    auto lockBounds = getCtrlTileBounds (4);
    g.setColour (isLocked ? juce::Colour (0xffe8347a) : juce::Colour (0xff242432));
    g.fillRoundedRectangle (lockBounds.toFloat(), 4.0f);
    g.setColour (isLocked ? juce::Colour (0xccffffff) : juce::Colour (0x99aaaacc));
    g.setFont (juce::Font (juce::FontOptions().withHeight (7.0f)));
    g.drawText ("LOCK", lockBounds.withHeight (14).translated (0, 4), juce::Justification::centred);
    drawLockIcon (g, lockBounds.toFloat().withTrimmedTop (12),
                  isLocked ? juce::Colours::white : juce::Colour (0xffddddee), isLocked);

    // ── Note tiles ─────────────────────────────────────────────────────────
    int stepCount  = proc_.stepCount_.load();
    int activeStep = proc_.currentStep_.load();

    // LED flash: bright white burst on beat, decays to tile colour over 150ms
    auto elapsed = (float) (juce::Time::getMillisecondCounter() - stepChangeTime_);
    float flashBrightness = (elapsed < 20.0f) ? 1.0f
                          : (elapsed < 150.0f) ? 1.0f - (elapsed - 20.0f) / 130.0f
                          : 0.0f;

    // Build preview order if dragging
    std::array<int, SqeletorProcessor::kMaxSteps> previewOrder{};
    for (int i = 0; i < SqeletorProcessor::kMaxSteps; ++i)
        previewOrder[static_cast<size_t> (i)] = i;

    if (draggingTile_ >= 0 && dragTargetSlot_ >= 0 && dragTargetSlot_ != draggingTile_)
    {
        // Swap preview
        std::swap (previewOrder[static_cast<size_t> (draggingTile_)],
                   previewOrder[static_cast<size_t> (dragTargetSlot_)]);
    }

    for (int slot = 0; slot < SqeletorProcessor::kMaxSteps; ++slot)
    {
        auto bounds  = getNoteTileBounds (slot);
        bool isEmpty = slot >= stepCount;

        if (isEmpty)
        {
            drawNoteTile (g, bounds, {}, false, true, false);
            continue;
        }

        // Skip the slot occupied by the dragged tile (it will be drawn at cursor)
        if (draggingTile_ >= 0 && slot == draggingTile_)
            continue;

        int stepIdx  = previewOrder[static_cast<size_t> (slot)];
        bool isActive = (stepIdx == activeStep) && draggingTile_ < 0;
        int  noteNum  = proc_.steps_[static_cast<size_t> (stepIdx)].noteNumber;
        bool isRest   = (noteNum < 0);
        juce::String noteName = isRest
            ? juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93")) // –
            : juce::MidiMessage::getMidiNoteName (noteNum, true, false, 4);
        auto colour = isRest ? juce::Colour (0xff555566)
                             : kNoteColours[static_cast<size_t> (noteNum) % 12];

        drawNoteTile (g, bounds, noteName, isActive, false, isRest, colour,
                      isActive ? flashBrightness : 0.0f);
    }

    // Draw dragged tile at cursor position
    if (draggingTile_ >= 0 && draggingTile_ < stepCount)
    {
        auto dragBounds = getNoteTileBounds (draggingTile_)
                              .withCentre (dragPos_);
        int  noteNum = proc_.steps_[static_cast<size_t> (draggingTile_)].noteNumber;
        bool isRest  = (noteNum < 0);
        juce::String noteName = isRest
            ? juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93"))
            : juce::MidiMessage::getMidiNoteName (noteNum, true, false, 4);

        auto colour = isRest ? juce::Colour (0xff555566)
                             : kNoteColours[static_cast<size_t> (noteNum) % 12];

        // Slight transparency while dragging
        g.saveState();
        g.setOpacity (0.85f);
        drawNoteTile (g, dragBounds, noteName, false, false, isRest, colour);
        g.restoreState();
    }
}

//==============================================================================
void SqeletorEditor::mouseDown (const juce::MouseEvent& event)
{
    auto pos = event.getPosition();

    // ── Control tiles ────────────────────────────────────────────────────
    for (int i = 0; i < kNumCtrl; ++i)
    {
        if (! getCtrlTileBounds (i).contains (pos)) continue;

        switch (i)
        {
            case 0: // RATE — cycle forward
            {
                auto* p = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("rate"));
                int next = (p->getIndex() + 1) % p->choices.size();
                float norm = static_cast<float> (next) / static_cast<float> (p->choices.size() - 1);
                p->setValueNotifyingHost (norm);
                break;
            }
            case 1: // MODE — cycle forward
            {
                auto* p = static_cast<juce::AudioParameterChoice*> (proc_.apvts.getParameter ("mode"));
                int next = (p->getIndex() + 1) % p->choices.size();
                float norm = static_cast<float> (next) / static_cast<float> (p->choices.size() - 1);
                p->setValueNotifyingHost (norm);
                break;
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
            case 4: // LOCK — toggle
            {
                auto* p = static_cast<juce::AudioParameterBool*> (proc_.apvts.getParameter ("locked"));
                p->setValueNotifyingHost (p->get() ? 0.0f : 1.0f);
                break;
            }
        }
        repaint();
        return;
    }

    // ── Note tile drag ───────────────────────────────────────────────────
    auto* recParam  = static_cast<juce::AudioParameterBool*> (proc_.apvts.getParameter ("recording"));
    auto* lockParam = static_cast<juce::AudioParameterBool*> (proc_.apvts.getParameter ("locked"));

    if (recParam->get() || lockParam->get()) return;

    int tileIndex = noteTileAt (pos);
    if (tileIndex < 0 || tileIndex >= proc_.stepCount_.load()) return;

    draggingTile_   = tileIndex;
    dragTargetSlot_ = tileIndex;
    dragPos_        = pos;
    repaint();
}

void SqeletorEditor::mouseDrag (const juce::MouseEvent& event)
{
    if (draggingTile_ < 0) return;

    dragPos_ = event.getPosition();

    // Find the nearest tile slot to the cursor
    int stepCount = proc_.stepCount_.load();
    int nearest = draggingTile_;
    int nearestDist = std::numeric_limits<int>::max();

    for (int i = 0; i < stepCount; ++i)
    {
        auto centre = getNoteTileBounds (i).getCentre();
        int dist = dragPos_.getDistanceFrom (centre);
        if (dist < nearestDist) { nearestDist = dist; nearest = i; }
    }

    if (nearest != dragTargetSlot_)
    {
        dragTargetSlot_ = nearest;
        repaint();
    }
    else
    {
        repaint(); // still repaint for smooth tile follow
    }
}

void SqeletorEditor::mouseUp (const juce::MouseEvent&)
{
    if (draggingTile_ < 0) return;

    int stepCount = proc_.stepCount_.load();

    if (dragTargetSlot_ != draggingTile_ && stepCount > 1)
    {
        // Swap the two steps
        for (int i = 0; i < stepCount; ++i)
            proc_.pendingSteps_[static_cast<size_t> (i)] =
                proc_.steps_[static_cast<size_t> (i)];

        std::swap (proc_.pendingSteps_[static_cast<size_t> (draggingTile_)],
                   proc_.pendingSteps_[static_cast<size_t> (dragTargetSlot_)]);

        proc_.reorderPending_.store (true);
    }

    draggingTile_   = -1;
    dragTargetSlot_ = -1;
    repaint();
}
