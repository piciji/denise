#pragma once

#include "debugger.h"

struct VicIIDebugger : Debugger {
    explicit VicIIDebugger( Emulator::Interface* emulator );

        struct Video : GUIKIT::HorizontalLayout {

        struct Wraper : GUIKIT::VerticalLayout {

            struct RegWrapper : GUIKIT::FramedVerticalLayout {
                struct Mode : GUIKIT::HorizontalLayout {
                    GUIKIT::Label modeLabel;
                    GUIKIT::LineEdit modeVal;
                    Mode();
                } mode;

                struct Registers : GUIKIT::HorizontalLayout {
                    GUIKIT::Label left;
                    GUIKIT::LineEdit leftVal;
                    GUIKIT::Label right;
                    GUIKIT::LineEdit rightVal;
                    Registers();
                };
                std::vector<Registers*> registers;

                RegWrapper();
            } regWrapper;

            struct Flags : GUIKIT::FramedHorizontalLayout {
                GUIKIT::Widget spacer;
                struct First : GUIKIT::VerticalLayout {
                    GUIKIT::CheckBox den;
                    GUIKIT::CheckBox badLine;
                    First();
                } first;

                struct Second : GUIKIT::VerticalLayout {
                    GUIKIT::CheckBox idle;
                    GUIKIT::CheckBox vblank;
                    Second();
                } second;

                struct Third : GUIKIT::VerticalLayout {
                    GUIKIT::CheckBox hFlop;
                    GUIKIT::CheckBox vFlop;
                    Third();
                } third;

                Flags();
            } flags;

            struct Intr : GUIKIT::FramedVerticalLayout {
                struct Latch : GUIKIT::HorizontalLayout {
                    GUIKIT::Label latch;
                    GUIKIT::LineEdit latchVal;
                    GUIKIT::CheckBox intLine;
                    GUIKIT::CheckBox lightPen;
                    GUIKIT::CheckBox ssCollision;
                    GUIKIT::CheckBox sfCollision;
                    GUIKIT::CheckBox raster;
                    Latch();
                } latch;

                struct Mask : GUIKIT::HorizontalLayout {
                    GUIKIT::Label mask;
                    GUIKIT::LineEdit maskVal;
                    GUIKIT::CheckBox intLine;
                    GUIKIT::CheckBox lightPen;
                    GUIKIT::CheckBox ssCollision;
                    GUIKIT::CheckBox sfCollision;
                    GUIKIT::CheckBox raster;
                    Mask();
                } mask;

                Intr();
            } intr;

            struct Lp : GUIKIT::FramedHorizontalLayout {
                GUIKIT::Widget spacer;
                GUIKIT::Label labelX;
                GUIKIT::LineEdit valX;
                GUIKIT::Label labelY;
                GUIKIT::LineEdit valY;
                GUIKIT::CheckBox line;
                GUIKIT::CheckBox latched;
                Lp();
            } lp;

            Wraper();
        } wraper;

        struct Sprites : GUIKIT::FramedVerticalLayout {

            struct Viewer : GUIKIT::FramedHorizontalLayout {
                GUIKIT::MultiSquareCanvas canvas;
                Viewer();
            } viewer;

            struct Selector : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::RadioBox spr[8];

                Selector();
            } selector;

            struct Props : GUIKIT::HorizontalLayout {
                GUIKIT::CheckBox priority;
                GUIKIT::CheckBox multiColor;
                GUIKIT::CheckBox sfCollision;
                GUIKIT::CheckBox ssCollision;

                Props();
            } props;

            struct Position : GUIKIT::VerticalLayout {
                struct First : GUIKIT::HorizontalLayout {
                    GUIKIT::Label labelX;
                    GUIKIT::LineEdit valX;
                    GUIKIT::CheckBox expandX;

                    GUIKIT::Label location;
                    GUIKIT::LineEdit locationVal;
                    First();
                } first;

                struct Second : GUIKIT::HorizontalLayout {
                    GUIKIT::Label labelY;
                    GUIKIT::LineEdit valY;
                    GUIKIT::CheckBox expandY;

                    GUIKIT::Label mcBase;
                    GUIKIT::LineEdit mcBaseVal;
                    Second();
                } second;

                Position();
            } position;

            Sprites();
        } sprites;

        Video(Debugger* debugger);
    };
    Video* video;

    auto screenIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto updateTheme() -> void override;
    auto closeTheme() -> void override;
    auto translateTheme() -> void override;
    auto initTheme() -> void override;

    auto updateView(LIBC64::DebuggerSnapshot& s) -> void;
    auto getSelectedSprite() -> unsigned;
};
