/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIUM-23A) */
/* SOLARIUM-23A as a plugin (VST3, AU on the Mac) and a standalone app: the engine (dsp/solaris.h), all its parameters
 * for the host (automation, saved with the project), its programs as the host's programs (the factory's, then your
 * own: files in Documents/SOLARIUM-23A/Presets), MIDI in (notes to the VCOs, pitch bend, the mod wheel to the
 * joystick's Y, CC 120/123), the host's tempo and position for the sequencer. */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "solaris.h"

class SolarisProcessor : public juce::AudioProcessor
{
public:
    SolarisProcessor();
    ~SolarisProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SOLARIUM-23A"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    /* A VST3 host (Cubase, FL Studio) sizes its program parameter when the plugin opens: a patch saved later lay
     * past that size, so the host answered the save with another patch. The VST3 host is told a fixed number
     * of slots instead; the slots past the real patches are empty. */
    static constexpr int HOST_PROGRAMS = 1024;
    int getNumPrograms() override { return wrapperType == wrapperType_VST3 ? juce::jmax(HOST_PROGRAMS, programCount()) : programCount(); }
    int programCount() const { return sol::numPresets() + (int)userPresets.size(); }
    int getCurrentProgram() override { return program; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    /* your own programs */
    struct UserPreset {
        juce::String name;
        std::vector<float> v;
    };
    std::vector<UserPreset> userPresets;
    static juce::File presetFolder();
    void loadUserPresets();
    /* the folder read again (after an import): the sound playing keeps its number */
    void rescanUser()
    {
        auto cur = getProgramName(program);
        loadUserPresets();
        if (getProgramName(program) != cur)
            for (int i = programCount(); --i >= 0;)
                if (getProgramName(i) == cur) {
                    program = i;
                    break;
                }
        updateHostDisplay(ChangeDetails().withProgramChanged(true));
    }
    int writePreset(const juce::String &name);   /* the sound as it is, kept under this name: its program number */

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String paramId(int i) { return sol::PARAMS[i].id; }

    juce::MidiKeyboardState keyboardState;   /* the touch plates (on PLATE_CHANNEL: tuned by their PLATE params) */
    static constexpr int PLATE_CHANNEL = 16;
    /* for the panel */
    std::atomic<float> light{0.0f}, lfo[2], voice[8];
    std::atomic<int> seqStep{0};
    std::atomic<float> uiScale{0.8f};
    std::atomic<int> uiFinish{0};            /* the panel's finish (Look.h), kept with the project */

private:
    sol::Engine engine;
    std::vector<std::atomic<float> *> raw;
    float sent[sol::NPARAMS] = {};
    bool fresh = true;
    int program = 0;
    void handleMidi(const juce::MidiMessage &m);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolarisProcessor)
};
