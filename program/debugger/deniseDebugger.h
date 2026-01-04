#pragma once

#include "debugger.h"

struct DeniseDebugger : Debugger {
    explicit DeniseDebugger( Emulator::Interface* emulator );

    struct Video : GUIKIT::HorizontalLayout {

        struct Wraper : GUIKIT::VerticalLayout {
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
                GUIKIT::CheckBox hires;
                GUIKIT::CheckBox shres;
                GUIKIT::CheckBox ham;
                GUIKIT::CheckBox dual;
                GUIKIT::CheckBox pf2OverPf1;
                Flags();
            } flags;

            struct FlagsECS : GUIKIT::HorizontalLayout {
                GUIKIT::Widget spacer;
                GUIKIT::CheckBox ecsena;
                GUIKIT::CheckBox brdblnk;
                GUIKIT::CheckBox extblken;
                FlagsECS();
            } flagsECS;

            struct Colors : GUIKIT::VerticalLayout {
                struct Row : GUIKIT::HorizontalLayout {
                    GUIKIT::Widget spacer;
                    GUIKIT::SquareCanvas cols[8];
                    Row();
                } rows[4];

                Colors();
            } colors;

            Wraper();
        } wraper;

        struct Sprites : GUIKIT::VerticalLayout {

            struct Viewer : GUIKIT::FramedHorizontalLayout {
                GUIKIT::MultiSquareCanvas canvas;
                Viewer();
            } viewer;

            struct Selector : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::RadioBox spr[8];

                Selector();
            } selector;

            struct Position : GUIKIT::HorizontalLayout {
                GUIKIT::Label labelVStart;
                GUIKIT::LineEdit valVStart;
                GUIKIT::Label labelVStop;
                GUIKIT::LineEdit valVStop;
                GUIKIT::Label labelH;
                GUIKIT::LineEdit valH;
                GUIKIT::CheckBox attached;

                Position();
            } position;

            struct Dat : GUIKIT::HorizontalLayout {
                GUIKIT::Label labelDatA;
                GUIKIT::LineEdit valDatA;
                GUIKIT::Label labelDatB;
                GUIKIT::LineEdit valDatB;
                Dat();
            } dat;

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

    auto updateView(LIBAMI::DebuggerSnapshot& s) -> void;
    auto getSelectedSprite() -> unsigned;
};
