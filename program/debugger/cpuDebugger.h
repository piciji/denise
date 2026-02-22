
#pragma once

#include "debugger.h"

#define LIST_INSTRUCTIONS 256
#define LIST_TRACES 512

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

        struct WatcherLayout : GUIKIT::VerticalLayout {
            GUIKIT::ListView list;

            GUIKIT::RadioBox breakPoint;

            struct MemoryAccessLayout : GUIKIT::HorizontalLayout {
                GUIKIT::RadioBox watchPoint;
                GUIKIT::CheckBox writeCheck;

                MemoryAccessLayout();
            } memoryAccessLayout;

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

            WatcherLayout();
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

            struct Options : GUIKIT::VerticalLayout {
                struct Address : GUIKIT::HorizontalLayout {
                    GUIKIT::LineEdit edit;
                    GUIKIT::ImageView view;

                    Address(Debugger* debugger);
                } address;

                struct Value : GUIKIT::HorizontalLayout {
                    GUIKIT::LineEdit edit;
                    GUIKIT::ImageView view;

                    Value(Debugger* debugger);
                } value;

                Options(Debugger* debugger);
            } options;

            State(Debugger* debugger);
        } state;

        CPU(Debugger* debugger);
    } *cpu = nullptr;

    struct BreakConditionLayout : GUIKIT::VerticalLayout {

        struct Expression : GUIKIT::HorizontalLayout {
            GUIKIT::CheckBox check;
            GUIKIT::ComboButton compareCombo;
            GUIKIT::LineEdit compareVal;

            Expression();
        } expression;

        struct HitCount : GUIKIT::HorizontalLayout {
            GUIKIT::CheckBox check;
            GUIKIT::ComboButton compareCombo;
            GUIKIT::LineEdit compareVal;

            HitCount();
        } hitCount;

        GUIKIT::MultilineEdit info;

        struct Control : GUIKIT::HorizontalLayout {
            GUIKIT::Widget spacer;
            GUIKIT::Button closeButton;

            Control();
        } control;

        BreakConditionLayout();
    };

    struct Instruction {
        unsigned addr;
        std::string disassembled;
        std::string data;
    };

    struct Trace {
        std::string disassembled;
        uint16_t flags;
    };

    struct Watcher {
        unsigned addr;
        std::string ident;
        DebuggerAction action;
        bool enabled;

        bool useHitCount = false;
        unsigned hitCount = 0;
        unsigned hitCountCompare = 0;

        bool useExpression = false;
        std::string expression;
        unsigned expressionCompare = 0;
    };

    std::optional<unsigned> currentInstRow;

    std::vector<Watcher> watchers;
    Instruction instructions[LIST_INSTRUCTIONS];
    Trace traces[LIST_TRACES];

    GUIKIT::Window* breakConditionWindow = nullptr;
    BreakConditionLayout* breakConditionLayout = nullptr;
    GUIKIT::Timer* unfocusTimer = nullptr;

    auto buildTheme() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto prepareTheme() -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;
    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;

    auto fetchTraces() -> void;
    auto fetchInstructions(unsigned addr) -> void;
    auto searchAddress(unsigned addr) -> void;

    auto updateInstructionList() -> void;
    auto updateTraceList() -> void;
    auto addToWatcherList(unsigned addr, DebuggerAction action, const std::string& ident = "") -> void;
    auto removeFromWatcherList(unsigned addr, DebuggerAction action) -> void;
    auto updateWatcherList() -> void;

    auto findWatcherBy(unsigned addr, DebuggerAction action) -> Watcher*;
    auto findWatcherRowBy(unsigned addr, DebuggerAction action) -> std::optional<unsigned>;

    auto updateBreakpointVisuals(Watcher* watcher) -> void;
    auto updateInstructionBreakpointVisuals(unsigned row, Watcher* watcher, bool preventColumResizing = false) -> void;
    auto updateWatcherBreakpointVisuals(unsigned row, Watcher* watcher, bool preventColumResizing = false) -> void;

    auto removeInstructionBreakpoint(unsigned row) -> void;
    auto findInstructionRowBy(unsigned addr) -> std::optional<unsigned>;

    auto updateCpuFlags(const char* flagIdent, unsigned flags) -> void;
    auto updateWatcherSelection() -> void;

    auto update68k(LIBAMI::DebuggerSnapshot& s) -> void;
    auto update6510(LIBC64::DebuggerSnapshot& s) -> void;
    auto update65816(LIBC64::DebuggerSnapshot& s) -> void;

    auto getCpuType() -> DebuggerTheme;
    auto memChanged() -> void;

    auto createWatchpointConditionOverlay(Watcher* watcher, GUIKIT::Position position) -> void;
    auto updateWatchpointCondition(Watcher& watcher) -> bool;
};
