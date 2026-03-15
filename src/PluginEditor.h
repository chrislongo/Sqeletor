#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SqeletorEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit SqeletorEditor (SqeletorProcessor&);
    ~SqeletorEditor() override = default;

    void paint      (juce::Graphics&) override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    // Layout
    juce::Rectangle<int> getCtrlTileBounds (int index) const;
    juce::Rectangle<int> getNoteTileBounds (int index) const;
    int noteTileAt (juce::Point<int> pos) const; // -1 if none

    // Drawing
    void drawCtrlTile  (juce::Graphics&, juce::Rectangle<int>,
                        const juce::String& label, const juce::String& value,
                        bool active) const;
    void drawNoteTile  (juce::Graphics&, juce::Rectangle<int>,
                        const juce::String& noteName,
                        bool isActive, bool isEmpty, bool isRest,
                        juce::Colour tileColour = juce::Colours::white,
                        float flashBrightness = 0.0f) const;
    void drawLockIcon  (juce::Graphics&, juce::Rectangle<float>,
                        juce::Colour colour, bool locked) const;

    SqeletorProcessor& proc_;
    int lastDisplayedStep_  { -1 };
    int lastDisplayedCount_ { -1 };
    juce::uint32 stepChangeTime_ { 0 };

    // Drag state
    int draggingTile_  { -1 };
    int dragTargetSlot_{ -1 };
    juce::Point<int> dragPos_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqeletorEditor)
};
