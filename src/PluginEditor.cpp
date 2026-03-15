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
        lastDisplayedStep_  = step;
        lastDisplayedCount_ = count;
        repaint();
    }
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
    // Active = near-black fill; inactive = white fill on grey panel
    g.setColour (active ? juce::Colour (0xff111111) : juce::Colours::white);
    g.fillRoundedRectangle (bounds.toFloat(), 4.0f);

    // Tiny label
    g.setColour (active ? juce::Colour (0xaaffffff) : juce::Colour (0x88000000));
    g.setFont (juce::Font (juce::FontOptions().withHeight (7.0f)));
    g.drawText (label, bounds.withHeight (14).translated (0, 4), juce::Justification::centred);

    // Value
    g.setColour (active ? juce::Colours::white : juce::Colour (0xff111111));
    float fontSize = (value.length() > 4) ? 12.0f : (value.length() > 3) ? 14.0f : 18.0f;
    g.setFont (juce::Font (juce::FontOptions().withHeight (fontSize)));
    g.drawText (value, bounds.withTrimmedTop (10), juce::Justification::centred);
}

void SqeletorEditor::drawLockIcon (juce::Graphics& g,
                                    juce::Rectangle<float> b,
                                    juce::Colour colour,
                                    bool locked) const
{
    float cx   = b.getCentreX();
    float bw   = b.getWidth() * 0.55f;
    float bodyTop = b.getY() + b.getHeight() * 0.47f;
    float bodyH   = b.getHeight() * 0.48f;

    // Body
    juce::Rectangle<float> body { cx - bw * 0.5f, bodyTop, bw, bodyH };
    g.setColour (colour);
    g.fillRoundedRectangle (body, 2.0f);

    // Keyhole dot
    g.setColour (colour.contrasting (0.6f));
    g.fillEllipse (cx - 2.5f, body.getCentreY() - 2.5f, 5.0f, 5.0f);

    // Shackle
    float sw = bw * 0.7f;
    float sh = b.getHeight() * 0.42f;
    float sy = b.getY() + b.getHeight() * 0.04f;

    juce::Path shackle;
    float ox = locked ? 0.0f : sw * 0.35f; // offset shackle right when open
    shackle.addArc (cx - sw * 0.5f + ox, sy, sw, sh,
                    juce::MathConstants<float>::pi,
                    juce::MathConstants<float>::twoPi, true);

    g.setColour (colour);
    g.strokePath (shackle, juce::PathStrokeType (2.5f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
}

void SqeletorEditor::drawNoteTile (juce::Graphics& g,
                                    juce::Rectangle<int> bounds,
                                    const juce::String& noteName,
                                    bool isActive,
                                    bool isEmpty,
                                    bool isRest) const
{
    if (isEmpty)
    {
        g.setColour (juce::Colour (0x44000000));
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 4.0f, 1.0f);
        return;
    }

    if (isActive)
    {
        // Active: near-black fill, white text — same "on" state as other Corvid controls
        g.setColour (juce::Colour (0xff111111));
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
    }
    else
    {
        // Inactive filled tile: white on grey panel
        g.setColour (juce::Colours::white);
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
    }

    auto textColour = isActive ? juce::Colours::white
                               : (isRest ? juce::Colour (0x55000000) : juce::Colour (0xff111111));
    g.setColour (textColour);

    float fontSize = (noteName.length() > 1) ? 62.0f : 88.0f;
    g.setFont (juce::Font (juce::FontOptions().withHeight (fontSize)));
    g.drawText (noteName, bounds, juce::Justification::centred);
}

//==============================================================================
void SqeletorEditor::paint (juce::Graphics& g)
{
    // Corvid house style: silver-grey panel with subtle top-to-bottom gradient
    g.fillAll (juce::Colour (0xffd8d8d8));
    juce::ColourGradient overlay (juce::Colour (0x18ffffff), 0.0f, 0.0f,
                                   juce::Colour (0x18000000), 0.0f, (float) getHeight(), false);
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
    g.setColour (isLocked ? juce::Colour (0xff111111) : juce::Colours::white);
    g.fillRoundedRectangle (lockBounds.toFloat(), 4.0f);
    g.setColour (isLocked ? juce::Colour (0xaaffffff) : juce::Colour (0x88000000));
    g.setFont (juce::Font (juce::FontOptions().withHeight (7.0f)));
    g.drawText ("LOCK", lockBounds.withHeight (14).translated (0, 4), juce::Justification::centred);
    drawLockIcon (g, lockBounds.toFloat().withTrimmedTop (12),
                  isLocked ? juce::Colours::white : juce::Colour (0xff111111), isLocked);

    // ── Note tiles ─────────────────────────────────────────────────────────
    int stepCount  = proc_.stepCount_.load();
    int activeStep = proc_.currentStep_.load();

    // Build preview order if dragging
    std::array<int, SqeletorProcessor::kMaxSteps> previewOrder{};
    for (int i = 0; i < SqeletorProcessor::kMaxSteps; ++i)
        previewOrder[static_cast<size_t> (i)] = i;

    if (draggingTile_ >= 0 && dragTargetSlot_ >= 0 && dragTargetSlot_ != draggingTile_)
    {
        // Insert-and-shift preview
        std::vector<int> order;
        for (int i = 0; i < stepCount; ++i) order.push_back (i);
        order.erase (order.begin() + draggingTile_);
        int insertAt = dragTargetSlot_;
        if (insertAt > draggingTile_) --insertAt;
        insertAt = juce::jlimit (0, static_cast<int> (order.size()), insertAt);
        order.insert (order.begin() + insertAt, draggingTile_);
        for (int i = 0; i < stepCount; ++i)
            previewOrder[static_cast<size_t> (i)] = order[static_cast<size_t> (i)];
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

        drawNoteTile (g, bounds, noteName, isActive, false, isRest);
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

        // Slight transparency while dragging
        g.saveState();
        g.setOpacity (0.85f);
        drawNoteTile (g, dragBounds, noteName, false, false, isRest);
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
        // Build reordered step array using insert-and-shift
        std::vector<int> order;
        for (int i = 0; i < stepCount; ++i) order.push_back (i);
        order.erase (order.begin() + draggingTile_);

        int insertAt = dragTargetSlot_;
        if (insertAt > draggingTile_) --insertAt;
        insertAt = juce::jlimit (0, static_cast<int> (order.size()), insertAt);
        order.insert (order.begin() + insertAt, draggingTile_);

        for (int i = 0; i < stepCount; ++i)
            proc_.pendingSteps_[static_cast<size_t> (i)] =
                proc_.steps_[static_cast<size_t> (order[static_cast<size_t> (i)])];

        proc_.reorderPending_.store (true);
    }

    draggingTile_   = -1;
    dragTargetSlot_ = -1;
    repaint();
}
