
#pragma once

#include "debugger.h"
#include "watcherHelper.h"
#include <optional>
#include <string>

#define LIST_INSTRUCTIONS 256
#define LIST_TRACES 512

struct CpuDebugger : Debugger {

    explicit CpuDebugger( Emulator::Interface* emulator );

    ~CpuDebugger() override;

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
                GUIKIT::LineEdit endAddress;
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

    struct C64RdyControl : GUIKIT::HorizontalLayout {
        GUIKIT::Button rdyButton;

        C64RdyControl();
    } *c64RdyControl = nullptr;

    struct Instruction {
        unsigned addr;
        std::string disassembled;
        std::string data;
    };

    struct Trace {
        std::string disassembled;
        uint16_t flags;
    };

    std::optional<unsigned> currentInstRow;

    Instruction instructions[LIST_INSTRUCTIONS];
    Trace traces[LIST_TRACES];
    WatcherHelper watcherHelper;

    auto buildTheme() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto prepareTheme(bool external) -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;
    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;

    auto fetchTraces() -> void;
    auto fetchInstructions(unsigned addr) -> void;
    auto searchAddress(unsigned addr) -> void;

    auto updateInstructionList() -> void;
    auto updateTraceList() -> void;

    auto updateBreakpointVisuals(DbgWatcher* watcher) -> void override;

    auto findInstructionRowBy(unsigned addr) -> std::optional<unsigned>;

    auto updateCpuFlags(const char* flagIdent, unsigned flags) -> void;
    auto updateWatcherSelection() -> void;

    auto update68k(LIBAMI::DebuggerSnapshot& s) -> void;
    auto update6510(LIBC64::DebuggerSnapshot& s) -> void;
    auto update6502(LIBC64::DebuggerSnapshot& s) -> void;
    auto update65816(LIBC64::DebuggerSnapshot& s) -> void;

    auto memChanged() -> void;

    auto buildControl() -> GUIKIT::Layout* override;

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::CPU; }

    auto addEntry(unsigned address, unsigned endAddress, DebuggerAction action) -> void;
    auto deleteEntry(unsigned address, DebuggerAction action) -> void;
    auto enableEntry(unsigned address, DebuggerAction action, bool enable) -> void;
    auto addCondition(unsigned address, DebuggerAction action, const std::string& condition) -> bool;

    virtual auto getDriveId() -> unsigned { return 0; }
};
