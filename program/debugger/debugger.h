
#pragma once

#include "../../guikit/api.h"
#include "../../emulation/interface.h"

typedef Emulator::Interface::DebuggerAction DebuggerAction;
typedef Emulator::Interface::DebuggerTheme DebuggerTheme;

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
        CPU, SCPU, Memory, MemorySCPU, CIA, Video, DMA,
    } mode;

    Debugger( Emulator::Interface* emulator, Mode mode );
    virtual ~Debugger();

    Emulator::Interface* emulator;
    GUIKIT::Settings* settings = nullptr;

    GUIKIT::Image addImg;
    GUIKIT::Image trashImg;
    GUIKIT::Image breakEnableImg;
    GUIKIT::Image breakDisableImg;
    GUIKIT::Image breakEnableSmallImg;
    GUIKIT::Image breakDisableSmallImg;
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
    GUIKIT::Image editImg;
    GUIKIT::Image checkedImg;

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

    Emulator::Interface::DebuggerSnapshot* snapshot = nullptr;
    static GUIKIT::Timer* timerVisibility;
    GUIKIT::VerticalLayout layout;

    auto build() -> void;
    virtual auto saveIdent() -> std::string = 0;
    virtual auto titleIdent() -> std::string = 0;
    virtual auto buildTheme() -> GUIKIT::Layout* = 0;
    virtual auto searchTheme(unsigned addr) -> void {}
    virtual auto translateTheme() -> void {}
    virtual auto updateTheme() -> void {}
    virtual auto prepareTheme() -> void {}
    virtual auto initTheme() -> void {}
    virtual auto closeTheme() -> void {}
    virtual auto buildControl() -> GUIKIT::Layout* { return nullptr; }

    auto translate() -> void;
    auto updateControl(uint16_t v, uint8_t h) -> void;

    static auto Callback(Emulator::Interface::DebuggerSnapshot* snapshot) -> void;
    static auto Callback() -> void;
    auto updateToolboxVisibility() -> void;
    auto makeVisible() -> void;

    auto isC64() -> bool;
    auto isAmiga() -> bool;
    auto isCpuMode() const -> bool { return mode == Mode::CPU || mode == Mode::SCPU; }
    auto isMemMode() const -> bool { return mode == Mode::Memory || mode == Mode::MemorySCPU; }
    auto isCiaMode() const -> bool { return mode == Mode::CIA; }

    static auto hex( uint32_t val, int length = -1 ) -> std::string;
    static auto updateReg(GUIKIT::LineEdit& reg, unsigned val) -> void;
    static auto updateReg(GUIKIT::CheckBox& reg, bool state) -> void;
    static auto updateReg(GUIKIT::LineEdit& widget, const std::string& text, unsigned ident) -> void;

    static auto isPaused() -> bool;
    static auto stepOut(Emulator::Interface* emulator) -> void;
    static auto stepInto(Emulator::Interface* emulator) -> void;
    static auto stepOver(Emulator::Interface* emulator) -> void;
    static auto stepLine(Emulator::Interface* emulator) -> void;
    static auto stepFrame(Emulator::Interface* emulator) -> void;
    static auto resume(Emulator::Interface* emulator) -> void;
    static auto reset() -> void;
    static auto getWidth(unsigned length, bool editField, bool bigger = false) -> unsigned;
};
