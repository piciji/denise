
#pragma once

#include "debugger.h"

struct AudioDebugger : Debugger {
    explicit AudioDebugger( Emulator::Interface* emulator );
    ~AudioDebugger() override {
        themeLayout = nullptr;
    }

    struct Chip : GUIKIT::VerticalLayout {
        struct Top : GUIKIT::HorizontalLayout {
            struct Voice : GUIKIT::FramedVerticalLayout {
                struct Wave : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::CheckBox noise;
                    GUIKIT::CheckBox pulse;
                    GUIKIT::CheckBox saw;
                    GUIKIT::CheckBox tri;
                    Wave();
                } wave;

                struct Frequency : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;
                    Frequency();
                } frequency;

                struct PulseWidth : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;
                    PulseWidth();
                } pulseWidth;

                struct Adsr : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit editA;
                    GUIKIT::LineEdit editD;
                    GUIKIT::LineEdit editS;
                    GUIKIT::LineEdit editR;
                    Adsr();
                } adsr;

                struct Control : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::CheckBox test;
                    GUIKIT::CheckBox ring;
                    GUIKIT::CheckBox sync;
                    GUIKIT::CheckBox gate;

                    Control();
                } control;

                Voice();
            } voices[3];

            Top();
        } top;

        struct Bottom : GUIKIT::HorizontalLayout {
            GUIKIT::Widget spacerL;

            struct Mixer : GUIKIT::FramedVerticalLayout {
                struct Mode : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::CheckBox highPass;
                    GUIKIT::CheckBox bandPass;
                    GUIKIT::CheckBox lowPass;
                    Mode();
                } mode;

                struct Filter : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::CheckBox voice3;
                    GUIKIT::CheckBox voice2;
                    GUIKIT::CheckBox voice1;
                    Filter();
                } filter;

                struct Params : GUIKIT::HorizontalLayout {
                    GUIKIT::Label labelCutoff;
                    GUIKIT::LineEdit editCutoff;
                    GUIKIT::Label labelResonance;
                    GUIKIT::LineEdit editResonance;
                    Params();
                } params;

                Mixer();
            } mixer;

            struct Misc : GUIKIT::FramedVerticalLayout {
                struct Volume : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;
                    GUIKIT::CheckBox disableVoice3;
                    Volume();
                } volume;

                struct Pot : GUIKIT::HorizontalLayout {
                    GUIKIT::Label labelX;
                    GUIKIT::LineEdit editX;
                    GUIKIT::Label labelY;
                    GUIKIT::LineEdit editY;
                    Pot();
                } pot;

                Misc();
            } misc;

            GUIKIT::Widget spacerR;

            Bottom();
        } bottom;

        Chip();
    } chips[8];

    GUIKIT::TabFrameLayout tab;

    auto updateSID(LIBC64::DebuggerSnapshot& snap) -> void;

    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto updateTheme() -> void override;
    auto closeTheme() -> void override;
    auto translateTheme() -> void override;
    auto initTheme() -> void override;
};
