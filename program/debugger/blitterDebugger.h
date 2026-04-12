
#pragma once

#include "debugger.h"

struct BlitterDebugger : Debugger {
    explicit BlitterDebugger( Emulator::Interface* emulator );

    struct Blitter : GUIKIT::HorizontalLayout {
        struct ColLeft : GUIKIT::VerticalLayout {
            struct Control : GUIKIT::FramedVerticalLayout {
                struct BltCon : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit editShift;
                    GUIKIT::LineEdit editChannel;
                    GUIKIT::LineEdit editControl;

                    BltCon();
                } bltCon0, bltCon1;

                struct BltSize: GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;
                    GUIKIT::Label labelCur;
                    GUIKIT::LineEdit editCur;

                    BltSize();
                } bltSizeW, bltSizeH;

                Control();
            } control;

            struct Flags : GUIKIT::FramedHorizontalLayout {
                GUIKIT::Widget spacer;

                struct Block : GUIKIT::VerticalLayout {
                    GUIKIT::CheckBox flag1;
                    GUIKIT::CheckBox flag2;
                    Block();
                };
                std::vector<Block*> blocks;

                Flags();
            } flags;

            struct BltD : GUIKIT::FramedVerticalLayout {
                struct Data : GUIKIT::HorizontalLayout {
                    GUIKIT::CheckBox check;
                    GUIKIT::LineEdit edit;

                    Data();
                } data;

                struct Fill : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;

                    Fill();
                } fillIn, fillOut;

                BltD();
            } bltD;

            ColLeft();
        } colLeft;

        struct ColCenter : GUIKIT::VerticalLayout {
            struct BltA : GUIKIT::FramedVerticalLayout {
                struct Data : GUIKIT::HorizontalLayout {
                    GUIKIT::CheckBox check;
                    GUIKIT::LineEdit edit;
                    GUIKIT::Label labelOld;
                    GUIKIT::LineEdit editOld;

                    Data();
                } data;

                struct BltWM : GUIKIT::HorizontalLayout {
                    GUIKIT::CheckBox check;
                    GUIKIT::LineEdit edit;

                    BltWM();
                } first, last;

                struct Barrel : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;

                    Barrel();
                } barrel;

                BltA();
            } bltA;

            struct BltB : GUIKIT::FramedVerticalLayout {
                struct Data : GUIKIT::HorizontalLayout {
                    GUIKIT::CheckBox check;
                    GUIKIT::LineEdit edit;
                    GUIKIT::Label labelOld;
                    GUIKIT::LineEdit editOld;

                    Data();
                } data;

                struct Barrel : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;

                    Barrel();
                } barrel;

                BltB();
            } bltB;

            struct BltC : GUIKIT::FramedHorizontalLayout {
                GUIKIT::CheckBox check;
                GUIKIT::LineEdit edit;

                BltC();
            } bltC;

            ColCenter();
        } colCenter;

        struct Minterm : GUIKIT::FramedVerticalLayout {
            struct Entry : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::CheckBox check;
                GUIKIT::LineEdit edit;
                Entry(bool useCheck);
            };

            std::vector<Entry*> entries;
            Minterm();
        } minterm;

        Blitter();
    } *blitter = nullptr;

    struct BlitterControl : GUIKIT::HorizontalLayout {
        GUIKIT::Button softStopButton;
        GUIKIT::Widget spacer;

        BlitterControl();
    } *blitterControl = nullptr;

    static auto binaryLength() -> unsigned;

    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;
    auto buildControl() -> GUIKIT::Layout* override;
};
