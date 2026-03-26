
#pragma once

#include "debugger.h"

struct DmaColor {
    unsigned color;
    uint8_t alpha = 0;
    bool enabled = false;
};

struct DmaDebugger : Debugger {
    explicit DmaDebugger( Emulator::Interface* emulator );
    ~DmaDebugger() override;

    struct Dma : GUIKIT::HorizontalLayout {

        struct Legend : GUIKIT::VerticalLayout {
            GUIKIT::Widget spacer;
            GUIKIT::Label dma;
            GUIKIT::Label dmaAddr;
            GUIKIT::Label dmaData;
            GUIKIT::Label mnemonic;
            GUIKIT::Label cpu;
            GUIKIT::Label cpuAddr;
            GUIKIT::Label cpuData;

            struct Watcher : GUIKIT::HorizontalLayout {
                GUIKIT::Widget spacer;
                GUIKIT::Button button;
                GUIKIT::LineEdit edit;
                unsigned position;

                Watcher();
            } watchers[4];

            Watcher* currentWatcher = nullptr;

            Legend();
        } legend;

        struct DmaLine : GUIKIT::FramedVerticalLayout {
            GUIKIT::LogicViewer viewer;

            DmaLine(DmaDebugger* debugger);
        } dmaLine;

        struct DmaFrame : GUIKIT::FramedVerticalLayout {
            GUIKIT::CheckBox showUsage;

            struct BusUsage : GUIKIT::HorizontalLayout {
                GUIKIT::CheckBox enableUsage;
                GUIKIT::SquareCanvas canvas;

                BusUsage();
            };
            std::vector<BusUsage*> usages;

            GUIKIT::HorizontalSlider slider;
            DmaFrame(DmaDebugger* debugger);
        } dmaFrame;


        Dma(DmaDebugger* debugger);
    } *dma = nullptr;

    struct DmaControl : GUIKIT::HorizontalLayout {
        GUIKIT::Widget spacer;
        GUIKIT::Button rdyButton;

        DmaControl(DmaDebugger* debugger);
    } *dmaControl = nullptr;

    GUIKIT::Menu watcherMenu;
    std::vector<GUIKIT::MenuItem*> watchItems;

    DmaColor dmaColors[0xf];

    constexpr static unsigned defaultColor[0xf] = {
        0,        // no BUS activity
        0xFFD700, // BPL
        0x3CC464, // Sprites
        0x0000FF, // Blitter
        0x800080, // Copper
        0x00FFFF, // CPU
        0xA9A9A9, // Refresh
        0xFFA500, // Disk
        0x808040, // Audio
        0xFF0000, // BLT-COP conflict
        0xFF00FF, // BLT-SPR conflict
        0xFF8080, // BPL-REF conflict
        0x800040, // BPL-Spr conflict
        0,
        0
    };

    GUIKIT::Timer scrollTimer;

    auto loadColors() -> void;

    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto updateTheme() -> void override;
    auto closeTheme() -> void override;
    auto translateTheme() -> void override;
    auto initTheme() -> void override;
    auto buildControl() -> GUIKIT::Layout* override;

    auto updateView(LIBAMI::DebuggerSnapshot& s) -> void;
    auto updateView(LIBC64::DebuggerSnapshot& s) -> void;
    auto updateColor(Dma::DmaFrame::BusUsage* busUsage, unsigned id, unsigned _col) -> void;
};
