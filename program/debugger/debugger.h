
#pragma once

#include "../../guikit/api.h"
#include "../../emulation/interface.h"

#define LIST_INSTRUCTIONS 256
#define LIST_TRACE 100
typedef Emulator::Interface::DebuggerAction DebuggerAction;

namespace LIBAMI {
    struct Interface;
    struct CpuSnapshot;
}

struct Debugger : GUIKIT::Window {
    Debugger( Emulator::Interface* emulator );
    ~Debugger();

    Emulator::Interface* emulator;
    GUIKIT::Settings* settings = nullptr;

    struct {
        unsigned addr = 0;
        Emulator::Interface::DebuggerAction action = Emulator::Interface::DebuggerAction::None;
        bool maybeModified = false;
    } last;

    GUIKIT::Image addImg;
    GUIKIT::Image trashImg;
    GUIKIT::Image breakEnableImg;
    GUIKIT::Image breakDisableImg;
    GUIKIT::Image searchImg;
    GUIKIT::Image nullImg;
    GUIKIT::Image pauseImg;
    GUIKIT::Image resumeImg;
    GUIKIT::Image stepOverImg;
    GUIKIT::Image stepIntoImg;
    GUIKIT::Image lineImg;
    GUIKIT::Image frameImg;
    GUIKIT::Image memoryImg;
    GUIKIT::Image exceptionImg;
    GUIKIT::Image clearImg;

    struct CPU68K : GUIKIT::HorizontalLayout {
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

            struct Adder : GUIKIT::HorizontalLayout {
                GUIKIT::LineEdit address;
                GUIKIT::Button add;
                Adder();
            } adder;

            GUIKIT::RadioBox breakPoint;
            GUIKIT::RadioBox watchPoint;
            GUIKIT::RadioBox exceptionPoint;

            Watcher();
        } watcher;

        struct State : GUIKIT::VerticalLayout {
            struct Registers : GUIKIT::HorizontalLayout {
                GUIKIT::Label left;
                GUIKIT::LineEdit leftVal;
                GUIKIT::Label right;
                GUIKIT::LineEdit rightVal;
                Registers();

            } registers[11];

            struct Flags : GUIKIT::HorizontalLayout {
                GUIKIT::Widget spacer;
                GUIKIT::Label flag[16];

                Flags();
            } flags;

            struct Trace : GUIKIT::HorizontalLayout {
                GUIKIT::CheckButton toggle;
                GUIKIT::Button clear;

                Trace();
            } trace;

            State();
        } state;


        CPU68K();
    } cpu68k;

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::Label position;
        GUIKIT::Button resume;
        GUIKIT::Button stepOver;
        GUIKIT::Button stepInto;
        GUIKIT::Button line;
        GUIKIT::Button frame;
        GUIKIT::LineEdit searchEdit;
        GUIKIT::ImageView search;
        Control();
    } control;

    struct Instruction {
        unsigned addr;
        std::string disassembled;
        std::string data;
    };

    struct Watcher {
        unsigned addr;
        DebuggerAction action;
        bool enabled;
    };

    std::vector<Watcher> watchers;
    Instruction instructions[LIST_INSTRUCTIONS];
    GUIKIT::Timer timer;

    GUIKIT::VerticalLayout layout;

    auto build() -> void;
    auto translate() -> void;
    auto update() -> void;
    auto cacheInstructions(unsigned addr) -> void;
    auto updateInstructionList() -> void;
    auto updateTraceList() -> void;
    auto addToWatcherList(unsigned addr, DebuggerAction action) -> void;
    auto removeFromWatcherList(unsigned addr, DebuggerAction action) -> void;
    auto updateWatcherList() -> void;

    auto findWatcherBy(unsigned addr, DebuggerAction action) -> Watcher*;
    auto findWatcherRowBy(unsigned addr, DebuggerAction action) -> std::optional<unsigned>;
    auto enableInstructionBreakpoint(unsigned row, bool state) -> void;
    auto removeInstructionBreakpoint(unsigned row) -> void;
    auto enableWatcher(unsigned row, bool state) -> void;
    auto findInstructionRowBy(unsigned addr) -> std::optional<unsigned>;

    auto debugCallback(DebuggerAction action, unsigned addr, bool maybeModified) -> void;
    auto debugCallback() -> void;
    auto updateToolboxVisibility() -> void;
    auto makeVisible() -> void;
    auto reset() -> void;

    auto updateAgnus(LIBAMI::Interface* amiEmu) -> void;
    auto update68k(LIBAMI::CpuSnapshot& s) -> void;

    auto hex( uint32_t val, int length = -1 ) -> std::string;
};
