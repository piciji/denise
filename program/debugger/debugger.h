
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
        CPU, SCPU, Memory, MemorySCPU, CIA,
    } mode;

    Debugger( Emulator::Interface* emulator, Mode mode );
    virtual ~Debugger();

    Emulator::Interface* emulator;
    GUIKIT::Settings* settings = nullptr;

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

        GUIKIT::CheckBox showTips;
        Control(Debugger* debugger);
    };
    Control* control = nullptr;

    static GUIKIT::Timer* timer;
    static GUIKIT::Timer* timerVisibility;

    GUIKIT::Layout* theme = nullptr;
    GUIKIT::VerticalLayout layout;

    auto build() -> void;
    virtual auto screenIdent() -> std::string = 0;
    virtual auto titleIdent() -> std::string = 0;
    virtual auto buildTheme() -> void = 0;
    virtual auto searchTheme(unsigned addr) -> void {}
    virtual auto translateTheme() -> void {}
    virtual auto updateTheme() -> void {}
    virtual auto initTheme() -> void {}
    virtual auto closeTheme() -> void {}
    virtual auto buildControl() -> GUIKIT::Layout* { return nullptr; }

    auto translate() -> void;
    auto update() -> void;

    static auto Callback(DebuggerAction action, unsigned addr, bool maybeModified) -> void;
    static auto Callback() -> void;
    auto updateToolboxVisibility() -> void;
    auto makeVisible() -> void;

    auto isC64() -> bool;
    auto isAmiga() -> bool;
    auto isCpuMode() const -> bool { return mode == Mode::CPU || mode == Mode::SCPU; }
    auto isMemMode() const -> bool { return mode == Mode::Memory || mode == Mode::MemorySCPU; }
    auto isCiaMode() const -> bool { return mode == Mode::CIA; }

    static auto hex( uint32_t val, int length = -1 ) -> std::string;

    static auto isPaused() -> bool;
    static auto stepOut(Emulator::Interface* emulator) -> void;
    static auto stepInto(Emulator::Interface* emulator) -> void;
    static auto stepOver(Emulator::Interface* emulator) -> void;
    static auto stepLine(Emulator::Interface* emulator) -> void;
    static auto stepFrame(Emulator::Interface* emulator) -> void;
    static auto resume(Emulator::Interface* emulator) -> void;
    static auto reset() -> void;
};
