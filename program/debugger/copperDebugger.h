
#pragma once

#include "debugger.h"
#include "watcherHelper.h"

#define LIST_COPPER_INSTRUCTIONS 1024u

struct DbgWatcher;

struct CopperDebugger : Debugger {
    explicit CopperDebugger( Emulator::Interface* emulator );
    ~CopperDebugger() override;

    struct Instructions {
        uint32_t addr;
        uint16_t data1;
        uint16_t data2;
    };

    struct Copper : GUIKIT::HorizontalLayout {
        struct List : GUIKIT::VerticalLayout {
            GUIKIT::ListView listView;

            struct Control : GUIKIT::HorizontalLayout {
                GUIKIT::Label labelCopLc;
                GUIKIT::LineEdit copLc;
                GUIKIT::Widget spacer;
                GUIKIT::LineEdit addrEdit;
                GUIKIT::ImageView addrView;
                GUIKIT::LineEdit valueEdit;
                GUIKIT::ImageView valueView;

                Control();
            } control;

            GUIKIT::Widget spacer;
            std::vector<Instructions> instructions;
            std::optional<unsigned> currentInstRow = std::nullopt;
            uint8_t* memory = nullptr;
            unsigned memorySize = 0;
            unsigned startAddr = 0;
            bool dirty = true;
            bool inUse = false;

            List();
        } lists[2];

        struct Watcher : GUIKIT::VerticalLayout {
            GUIKIT::ListView listView;

            struct TypeLayout : GUIKIT::HorizontalLayout {
                GUIKIT::RadioBox watchPoint;
                GUIKIT::RadioBox breakPoint;

                TypeLayout();
            } typeLayout;

            struct Adder : GUIKIT::HorizontalLayout {
                GUIKIT::LineEdit address;
                GUIKIT::Button add;
                Adder();
            } adder;

            struct Control : GUIKIT::HorizontalLayout {
                GUIKIT::Label labelCopPc;

                GUIKIT::LineEdit copPc;
                GUIKIT::CheckBox cdang;

                Control();
            } control;

            Watcher();
        } watcher;

        Copper();
    } *copper;

    struct CopperControl : GUIKIT::HorizontalLayout {
        GUIKIT::Button softStopButton;
        GUIKIT::Widget spacer;
        GUIKIT::CheckBox symbolic;

        CopperControl();
    } *copperControl = nullptr;

    WatcherHelper watcherHelper;

    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto prepareTheme(bool external) -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;
    auto buildControl() -> GUIKIT::Layout* override;
    auto updateBreakpointVisuals(DbgWatcher* watcher) -> void override;

    auto findInstructionRowBy(Copper::List* list, unsigned addr) -> std::optional<unsigned>;
    auto updateInstructionList(Copper::List* list, bool forceUpdate = false) -> void;
    auto updateWatcherSelection() -> void;
    auto updateInstructionBreakpointVisualsInOtherList(Copper::List* lPtr, unsigned addr, DbgWatcher* watcher) -> void;
    auto updateInstructionViews(bool forceUpdate = false) -> void;
    auto searchAddress(Copper::List* list, unsigned addr) -> void;
    auto memChanged() -> void;
    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Copper; }
};
