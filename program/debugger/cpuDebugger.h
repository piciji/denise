
#pragma once

#include "debugger.h"

struct CpuDebugger : Debugger {

    explicit CpuDebugger( Emulator::Interface* emulator );
    explicit CpuDebugger( Emulator::Interface* emulator, Mode mode );

    struct CPU : GUIKIT::HorizontalLayout {
        GUIKIT::SwitchLayout switchLayout;

        struct InstructionLayout : GUIKIT::VerticalLayout {
            GUIKIT::ListView list;
            InstructionLayout();
        } instructionLayout;

        struct TraceLayout : GUIKIT::VerticalLayout {
            GUIKIT::ListView list;
            TraceLayout();
        } traceLayout;

        struct Watcher : GUIKIT::VerticalLayout {
            GUIKIT::ListView list;

            GUIKIT::RadioBox breakPoint;
            GUIKIT::RadioBox watchPoint;

            struct Adder : GUIKIT::HorizontalLayout {
                GUIKIT::LineEdit address;
                GUIKIT::Button add;
                Adder();
            } adder;

            struct ExcAdder : GUIKIT::HorizontalLayout {
                GUIKIT::ComboButton exceptionCombo;
                GUIKIT::Button add;
                ExcAdder();
            } excAdder;

            Watcher();
        } watcher;

        struct State : GUIKIT::VerticalLayout {
            struct Registers : GUIKIT::HorizontalLayout {
                GUIKIT::Label left;
                GUIKIT::LineEdit leftVal;
                GUIKIT::Label right;
                GUIKIT::LineEdit rightVal;
                Registers(Debugger* debugger);
            };
            std::vector<Registers*> registers;

            struct Flags : GUIKIT::HorizontalLayout {
                GUIKIT::Widget spacer;
                std::vector<GUIKIT::Label*> flag;

                Flags(Debugger* debugger);
            } flags;

            struct Trace : GUIKIT::HorizontalLayout {
                GUIKIT::CheckButton toggle;
                GUIKIT::Button clear;

                Trace();
            } trace;

            State(Debugger* debugger);
        } state;

        CPU(Debugger* debugger);
    };

    CPU* cpu = nullptr;

    struct Instruction {
        unsigned addr;
        std::string disassembled;
        std::string data;
    };

    struct Watcher {
        unsigned addr;
        std::string ident;
        DebuggerAction action;
        bool enabled;
    };

    struct {
        unsigned addr = 0;
        Emulator::Interface::DebuggerAction action = DebuggerAction::None;
        bool maybeModified = false;
    } last;

    std::vector<Watcher> watchers;
    Instruction instructions[LIST_INSTRUCTIONS];

    auto buildTheme() -> GUIKIT::Layout* override;
    auto searchTheme(unsigned addr) -> void override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;
    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;

    auto cacheInstructions(unsigned addr) -> void;
    auto updateInstructionList() -> void;
    auto updateTraceList() -> void;
    auto addToWatcherList(unsigned addr, DebuggerAction action, const std::string& ident = "") -> void;
    auto removeFromWatcherList(unsigned addr, DebuggerAction action) -> void;
    auto updateWatcherList() -> void;

    auto findWatcherBy(unsigned addr, DebuggerAction action) -> Watcher*;
    auto findWatcherRowBy(unsigned addr, DebuggerAction action) -> std::optional<unsigned>;
    auto enableInstructionBreakpoint(unsigned row, bool state) -> void;
    auto removeInstructionBreakpoint(unsigned row) -> void;
    auto enableWatcher(unsigned row, bool state) -> void;
    auto findInstructionRowBy(unsigned addr) -> std::optional<unsigned>;

    auto updateCpuFlags(const char* flagIdent, unsigned flags) -> void;
    auto updateWatcherSelection() -> void;

    auto update68k(LIBAMI::DebuggerSnapshot& s) -> void;
    auto update6510(LIBC64::DebuggerSnapshot& s) -> void;
    auto update65816(LIBC64::DebuggerSnapshot& s) -> void;

    auto getCpuType() -> Emulator::Interface::DebuggerChip;
};
