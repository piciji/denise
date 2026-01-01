
#pragma once

#include "debugger.h"

struct MemDebugger : Debugger {
    explicit MemDebugger( Emulator::Interface* emulator );
    explicit MemDebugger( Emulator::Interface* emulator, Mode mode );

    ~MemDebugger();

    struct Memory : GUIKIT::HorizontalLayout {
        GUIKIT::ListView bankList;
        GUIKIT::ListView pageList;

        Memory(Debugger* debugger);
    };

    Memory* memory = nullptr;

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

    C64MemControl* c64MemControl = nullptr;

    uint8_t bankListStore[256] = {0};
    uint8_t* memDump = nullptr;
    uint8_t* memDumpOld = nullptr;

    auto buildTheme() -> GUIKIT::Layout* override;
    auto buildControl() -> GUIKIT::Layout* override;
    auto searchTheme(unsigned addr) -> void override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto screenIdent() -> std::string override;
    auto titleIdent() -> std::string override;

    auto updateC64MemControl(uint8_t _mode, bool init = false) -> void;
    auto updateMemory(LIBAMI::DebuggerSnapshot& s) -> void;
    auto updateMemory(LIBC64::DebuggerSnapshot& s) -> void;
    template<typename T> auto loadMemoryBank(uint8_t bank, bool noColorChanges) -> void;
    static auto toAscii(const uint8_t* buf, int len, char* result, char pad = '.') -> void;
};
