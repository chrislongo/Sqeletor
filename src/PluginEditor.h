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
    juce::Rectangle<int> getCtrlTileBounds  (int index) const;
    juce::Rectangle<int> getNoteTileBounds  (int index) const;
    juce::Rectangle<int> getScaleTileBounds (int index) const;   // 0=Key, 1=Scale
    int noteTileAt (juce::Point<int> pos) const; // -1 if none

    // Drawing
    void drawCtrlTile  (juce::Graphics&, juce::Rectangle<int>,
                        const juce::String& label, const juce::String& value,
                        bool active) const;
    void drawNoteTile  (juce::Graphics&, juce::Rectangle<int>,
                        const juce::String& noteName,
                        bool isActive, bool isEmpty, bool isRest,
                        juce::Colour tileColour = juce::Colours::white,
                        float flashBrightness = 0.0f,
                        int octave = -100) const;

    SqeletorProcessor& proc_;
    int lastDisplayedStep_  { -1 };
    int lastDisplayedCount_ { -1 };
    juce::uint32 stepChangeTime_ { 0 };

    // Ctrl tile drag state (RATE / MODE)
    int ctrlDragIndex_   { -1 };
    int ctrlDragStartY_  {  0 };
    int ctrlDragBaseStep_{  0 };

    // Note tile drag state (X=pitch-class, Y=octave)
    int  dragTile_        { -1 };
    int  dragStartX_      {  0 };
    int  dragStartY_      {  0 };
    int  dragBaseIdx_     {  0 };   // pitch-class 0-12 (12=rest) at drag start
    int  dragBaseOctave_  {  0 };   // octave at drag start
    int  dragCurrentNote_ {  0 };   // -1 (rest) or 0-127, used by paint

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqeletorEditor)
};
