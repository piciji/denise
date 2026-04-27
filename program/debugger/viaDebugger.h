
#pragma once

#include "debugger.h"

struct ViaDebugger : Debugger {
    explicit ViaDebugger( Emulator::Interface* emulator );

    struct VIA : GUIKIT::HorizontalLayout {
        struct Chip : GUIKIT::FramedVerticalLayout {
            struct Port : GUIKIT::HorizontalLayout {
                GUIKIT::Label pr;
                GUIKIT::LineEdit prVal;
                GUIKIT::Label ddr;
                GUIKIT::LineEdit ddrVal;
                GUIKIT::CheckBox useLatch;
                Port();
            } port[2];

            struct PortIO : GUIKIT::HorizontalLayout {
                GUIKIT::Label portLabel;
                std::vector<GUIKIT::Label*> line;
                PortIO(uint8_t chipNr, uint8_t portNr, Debugger* debugger);
            } portIO[2];

            struct Timer : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::LineEdit val;
                GUIKIT::Label latch;
                GUIKIT::LineEdit latchVal;

                GUIKIT::CheckBox oneShot;
                GUIKIT::CheckBox pbOut;
                GUIKIT::CheckBox toggleOut;
                GUIKIT::CheckBox pb6Pulses;
                Timer();
            } timer[2];

            struct Intr : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::LineEdit val;
                GUIKIT::CheckBox ir;
                GUIKIT::CheckBox ta;
                GUIKIT::CheckBox tb;
                GUIKIT::CheckBox cb1;
                GUIKIT::CheckBox cb2;
                GUIKIT::CheckBox shift;
                GUIKIT::CheckBox ca1;
                GUIKIT::CheckBox ca2;
                Intr(bool isMask);
            } ifr, ier;

            struct CX1 : GUIKIT::HorizontalLayout {
                GUIKIT::Label labelA;
                GUIKIT::CheckBox checkPositiveA;
                GUIKIT::Label labelB;
                GUIKIT::CheckBox checkPositiveB;

                CX1();
            } cx1;

            struct CX2Out : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::CheckBox checkOutput;
                GUIKIT::RadioBox radioHandshake;
                GUIKIT::RadioBox radioPulse;
                GUIKIT::RadioBox radioLow;
                GUIKIT::RadioBox radioHigh;

                CX2Out();
            } ca2Out, cb2Out;

            struct CX2In : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::CheckBox checkInput;
                GUIKIT::CheckBox checkPositive;
                GUIKIT::CheckBox checkIndependent;

                CX2In();
            } ca2In, cb2In;

            struct Shifter : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::LineEdit editSdr;
                GUIKIT::Label labelShiftCount;
                GUIKIT::LineEdit editShiftCount;
                GUIKIT::CheckBox checkOutput;
                GUIKIT::RadioBox radioDisabled;
                GUIKIT::RadioBox radioTimer2;
                GUIKIT::RadioBox radioPhi2;
                GUIKIT::RadioBox radioExt;

                Shifter();
            } shifter;

            Chip(uint8_t chipNr, Debugger* debugger);
        } chip[2];

        VIA(Debugger* debugger);
    } *via = nullptr;

    template<typename T> auto updateVia(T& s) -> void;

    auto buildTheme() -> GUIKIT::Layout* override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;

    virtual auto getDriveId() -> unsigned { return 0; }
};
