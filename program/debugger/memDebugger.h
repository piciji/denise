
#pragma once

#include "debugger.h"

struct MemDebugger : Debugger {
    explicit MemDebugger( Emulator::Interface* emulator );
    explicit MemDebugger( Emulator::Interface* emulator, Mode mode );

    ~MemDebugger() override;

    struct Memory : GUIKIT::HorizontalLayout {
        GUIKIT::ListView bankList;
        GUIKIT::ListView pageList;

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

        Memory(Debugger* debugger);
    } *memory = nullptr;

    struct C64MemControl : GUIKIT::HorizontalLayout {
        GUIKIT::Widget spacer;

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
    } *c64MemControl = nullptr;

    uint8_t bankListStore[256] = {0};
    uint8_t* memDump = nullptr;
    uint8_t* memDumpOld = nullptr;

    auto buildTheme() -> GUIKIT::Layout* override;
    auto buildControl() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto prepareTheme() -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;
    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;

    auto searchAddress(unsigned addr) -> void;
    auto memChanged(bool noColorChanges = true) -> void;

    auto updateC64MemControl(uint8_t _mode, bool init = false) -> void;
    auto updateMemory(LIBAMI::DebuggerSnapshot& s) -> void;
    auto updateMemory(LIBC64::DebuggerSnapshot& s) -> void;
    template<typename T> auto loadMemoryBank(uint8_t bank, bool noColorChanges) -> void;
    template<typename T> auto fetchDump() -> void;
    static auto toAscii(const uint8_t* buf, int len, char* result, char pad = '.') -> void;
};
