
#pragma once

#include "../../guikit/api.h"
#include "../../emulation/interface.h"

#define LIST_INSTRUCTIONS 256

typedef Emulator::Interface::DebuggerAction DebuggerAction;

namespace LIBAMI {
    struct Interface;
    struct DebuggerSnapshot;
}

namespace LIBC64 {
    struct Interface;
    struct DebuggerSnapshot;
}

struct Debugger : GUIKIT::Window {
    enum class Mode {
        CPU, Memory
    } mode;

    Debugger( Emulator::Interface* emulator, Mode mode );
    ~Debugger();

    Emulator::Interface* emulator;
    GUIKIT::Settings* settings = nullptr;
    std::string screenIdent;

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
    GUIKIT::Image stepOutImg;
    GUIKIT::Image lineImg;
    GUIKIT::Image frameImg;
    GUIKIT::Image memoryImg;
    GUIKIT::Image exceptionImg;
    GUIKIT::Image clearImg;
    GUIKIT::Image offImg;
    GUIKIT::Image onImg;

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
                Registers();
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

    struct Memory : GUIKIT::HorizontalLayout {
        GUIKIT::ListView bankList;
        GUIKIT::ListView pageList;

        Memory(Debugger* debugger);
    };

    struct C64MemControl : GUIKIT::HorizontalLayout {

        struct Element : GUIKIT::HorizontalLayout {
            GUIKIT::ImageView imgView;
            GUIKIT::Label label;

            Element(Debugger* debugger);
        };

        struct Left : GUIKIT::VerticalLayout {
            Element exrom;
            Element game;

            Left(Debugger* debugger);
        } left;

        struct Middle : GUIKIT::VerticalLayout {
            Element charen;

            Middle(Debugger* debugger);
        } middle;

        struct Right : GUIKIT::VerticalLayout {
            Element loram;
            Element hiram;

            Right(Debugger* debugger);
        } right;

        C64MemControl(Debugger* debugger);
    };

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::Widget spacer;
        GUIKIT::Button resume;
        GUIKIT::Button stepOver;
        GUIKIT::Button stepInto;
        GUIKIT::Button stepOut;
        GUIKIT::Button line;
        GUIKIT::Button frame;
        GUIKIT::LineEdit searchEdit;
        GUIKIT::ImageView search;
        GUIKIT::Label position;

        C64MemControl* c64MemControl = nullptr;
        GUIKIT::CheckBox showTips;
        Control(Debugger* debugger);
    } control;

    uint8_t bankListStore[256] = {0};
    uint8_t* memDump = nullptr;
    uint8_t* memDumpOld = nullptr;

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

    std::vector<Watcher> watchers;
    Instruction instructions[LIST_INSTRUCTIONS];
    static GUIKIT::Timer* timer;
    static GUIKIT::Timer* timerVisibility;

    CPU* cpu = nullptr;
    Memory* memory = nullptr;

    GUIKIT::VerticalLayout layout;

    auto build() -> void;
    auto buildCPU() -> void;
    auto buildMem() -> void;
    auto translate() -> void;
    auto update() -> void;
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

    static auto Callback(DebuggerAction action, unsigned addr, bool maybeModified) -> void;
    static auto Callback() -> void;
    auto updateToolboxVisibility() -> void;
    auto makeVisible() -> void;

    auto update68k(LIBAMI::DebuggerSnapshot& s) -> void;
    auto update6510(LIBC64::DebuggerSnapshot& s) -> void;
    auto updateMemory(LIBAMI::DebuggerSnapshot& s) -> void;
    auto updateMemory(LIBC64::DebuggerSnapshot& s) -> void;
    auto loadMemoryBank16(uint8_t bank, bool swap) -> void;
    auto loadMemoryBank12(uint8_t bank, bool swap) -> void;
    auto updateCpuFlags(const char* flagIdent, unsigned flags) -> void;
    auto updateCpuReg(GUIKIT::LineEdit& reg, unsigned val) -> void;
    auto updateWatcherSelection() -> void;
    auto initWatchers() -> void;
    auto updateC64MemControl(uint8_t _mode, bool init = false) -> void;

    static auto hex( uint32_t val, int length = -1 ) -> std::string;
    static auto toAscii(const uint8_t* buf, int len, char* result, char pad = '.') -> void;

    static auto stepOut(Emulator::Interface* emulator) -> void;
    static auto stepInto(Emulator::Interface* emulator) -> void;
    static auto stepOver(Emulator::Interface* emulator) -> void;
    static auto stepLine(Emulator::Interface* emulator) -> void;
    static auto stepFrame(Emulator::Interface* emulator) -> void;
    static auto resume(Emulator::Interface* emulator) -> void;
    static auto reset() -> void;
};
