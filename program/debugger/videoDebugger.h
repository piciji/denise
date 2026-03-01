
#pragma once

#include "debugger.h"

struct VideoDebugger : Debugger {
    explicit VideoDebugger( Emulator::Interface* emulator );

    struct Video : GUIKIT::HorizontalLayout {

        struct Wraper : GUIKIT::VerticalLayout {

            struct RegWrapper : GUIKIT::FramedVerticalLayout {
                struct Mode : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit val;
                    Mode();
                } mode;

                struct Register : GUIKIT::HorizontalLayout {
                    GUIKIT::Label left;
                    GUIKIT::LineEdit leftVal;
                    GUIKIT::Label right;
                    GUIKIT::LineEdit rightVal;
                    Register();
                };
                std::vector<Register*> registers;

                RegWrapper(Debugger* debugger);
            } regWrapper;

            struct Flags : GUIKIT::FramedHorizontalLayout {
                GUIKIT::Widget spacer;

                struct Block : GUIKIT::VerticalLayout {
                    GUIKIT::CheckBox flag1;
                    GUIKIT::CheckBox flag2;
                    Block();
                };
                std::vector<Block*> blocks;

                Flags(Debugger* debugger);
            } flags;

            struct Colors : GUIKIT::FramedVerticalLayout {
                struct Row : GUIKIT::HorizontalLayout {
                    GUIKIT::Widget spacer;
                    GUIKIT::SquareCanvas cols[16];
                    Row();
                } rows[2];

                Colors();
            } colors;

            struct Intr : GUIKIT::FramedVerticalLayout {
                struct Row : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit val;
                    GUIKIT::CheckBox intLine;
                    std::vector<GUIKIT::CheckBox*> boxes;
                    Row(Debugger* debugger, bool isMask);
                } latch, mask;

                Intr(Debugger* debugger);
            } intr;

            Wraper(Debugger* debugger);
        } wraper;

        struct WraperRight : GUIKIT::VerticalLayout {
            struct Lightpen : GUIKIT::FramedVerticalLayout {
                struct Top : GUIKIT::HorizontalLayout {
                    GUIKIT::Label labelX;
                    GUIKIT::LineEdit valX;
                    GUIKIT::CheckBox line;
                    Top();
                } top;

                struct Bottom : GUIKIT::HorizontalLayout {
                    GUIKIT::Label labelY;
                    GUIKIT::LineEdit valY;
                    GUIKIT::CheckBox latched;
                    Bottom();
                } bottom;

                Lightpen(Debugger* debugger);
            } lightpen;

            WraperRight(Debugger* debugger);
        } wraperRight;

        struct Sprites : GUIKIT::FramedVerticalLayout {

            struct Viewer : GUIKIT::FramedHorizontalLayout {
                GUIKIT::MultiSquareCanvas canvas;
                Viewer(Debugger* debugger);
            } viewer;

            struct Selector : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::RadioBox spr[8];

                Selector();
            } selector;

            struct Flags : GUIKIT::HorizontalLayout {
                std::vector<GUIKIT::CheckBox*> boxes;

                Flags(Debugger* debugger);
            } flags;

            struct Position : GUIKIT::VerticalLayout {
                struct Direction : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit val;
                    GUIKIT::CheckBox flag;
                    GUIKIT::Label labelTo;
                    GUIKIT::LineEdit valTo;

                    GUIKIT::Label labelMemory;
                    GUIKIT::LineEdit memoryVal;
                    Direction(Debugger* debugger);
                } vertical, horizontal;

                Position(Debugger* debugger);
            } position;

            Sprites(Debugger* debugger);
        } sprites;

        Video(Debugger* debugger);
    } *video = nullptr;

    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto updateTheme() -> void override;
    auto closeTheme() -> void override;
    auto translateTheme() -> void override;
    auto initTheme() -> void override;

    auto updateView(LIBC64::DebuggerSnapshot& s) -> void;
    auto updateView(LIBAMI::DebuggerSnapshot& s) -> void;
    auto getSelectedSprite() -> unsigned;
};
