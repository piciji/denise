
#pragma once

#include "debugger.h"

struct CiaDebugger : Debugger {
    explicit CiaDebugger( Emulator::Interface* emulator, Mode mode = Mode::CIA );

    struct CIA : GUIKIT::HorizontalLayout {
        struct Chip : GUIKIT::FramedVerticalLayout {
            struct Port : GUIKIT::HorizontalLayout {
                GUIKIT::Label pr;
                GUIKIT::LineEdit prVal;
                GUIKIT::Label ddr;
                GUIKIT::LineEdit ddrVal;
                Port(uint8_t portNr);
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
                Timer(uint8_t portNr);
            } timer[2];

            struct IntData : GUIKIT::HorizontalLayout {
                GUIKIT::Label icr;
                GUIKIT::LineEdit icrVal;
                GUIKIT::CheckBox ir;
                GUIKIT::CheckBox flag;
                GUIKIT::CheckBox sp;
                GUIKIT::CheckBox alarm;
                GUIKIT::CheckBox tb;
                GUIKIT::CheckBox ta;
                IntData();
            } intData;

            struct IntMask : GUIKIT::HorizontalLayout {
                GUIKIT::Label mask;
                GUIKIT::LineEdit maskVal;
                GUIKIT::CheckBox ir;
                GUIKIT::CheckBox flag;
                GUIKIT::CheckBox sp;
                GUIKIT::CheckBox alarm;
                GUIKIT::CheckBox tb;
                GUIKIT::CheckBox ta;
                IntMask();
            } intMask;

            struct Tod24bit : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::LineEdit counter;
                GUIKIT::Label labelAlarm;
                GUIKIT::LineEdit counterAlarm;
                Tod24bit();
            } tod24bit;

            struct Shifter : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::LineEdit sdr;
                GUIKIT::Label labelShiftCount;
                GUIKIT::LineEdit shiftCount;
                Shifter();
            } shifter;

            Chip(uint8_t chipNr, Debugger* debugger);
        } chip[2];

        CIA(Debugger* debugger);
    };
    CIA* cia = nullptr;

    template<typename T> auto updateCia(T& s) -> void;

    auto buildTheme() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto screenIdent() -> std::string override;
    auto titleIdent() -> std::string override;
};
