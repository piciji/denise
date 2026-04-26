
#pragma once

#include "debugger.h"

struct PaulaDebugger : Debugger {
    explicit PaulaDebugger( Emulator::Interface* emulator );

    struct Paula : GUIKIT::HorizontalLayout {
        struct Intr : GUIKIT::FramedVerticalLayout {
            struct Header : GUIKIT::HorizontalLayout {
                GUIKIT::Widget spacer;
                GUIKIT::Label label;
                GUIKIT::Label labelR;
                GUIKIT::Widget spacerR;
                Header();
            } header;

            struct Body : GUIKIT::HorizontalLayout {
                GUIKIT::Widget spacer;
                GUIKIT::LineEdit edit;
                GUIKIT::LineEdit editR;
                GUIKIT::Widget spacerR;
                Body();
            } body;

            struct Entry : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::CheckBox check;
                GUIKIT::CheckBox checkR;

                Entry();
            };
            std::vector<Entry*> entries;

            Intr();
        } intr;

        struct Aud : GUIKIT::VerticalLayout {
            struct Cha : GUIKIT::FramedVerticalLayout {
                struct Line1 : GUIKIT::HorizontalLayout {
                    GUIKIT::Label labelLen;
                    GUIKIT::LineEdit editLen;
                    GUIKIT::ImageView imageLen;
                    GUIKIT::LineEdit editCurLen;
                    GUIKIT::Label labelPer;
                    GUIKIT::LineEdit editPer;
                    GUIKIT::ImageView imagePer;
                    GUIKIT::LineEdit editCurPer;
                    Line1();
                } line1;

                struct Line2 : GUIKIT::HorizontalLayout {
                    GUIKIT::Label labelDat;
                    GUIKIT::LineEdit editDat;
                    GUIKIT::ImageView imageDat;
                    GUIKIT::LineEdit editCurDat;
                    GUIKIT::Label labelVol;
                    GUIKIT::LineEdit editVol;
                    GUIKIT::ImageView imageVol;
                    GUIKIT::LineEdit editCurVol;
                    Line2();
                } line2;

                struct Line3 : GUIKIT::HorizontalLayout {
                    GUIKIT::CheckBox useV;
                    GUIKIT::CheckBox useP;
                    GUIKIT::Label state;
                    GUIKIT::RadioBox radios[6];
                    Line3();
                } line3;

                Cha();
            } chas[4];

            Aud();
        } aud;

        struct Fdc : GUIKIT::FramedVerticalLayout {
            struct Entry : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::LineEdit edit;
                GUIKIT::Label labelR;
                GUIKIT::LineEdit editR;

                Entry();
            };

            struct Flags : GUIKIT::HorizontalLayout {
                GUIKIT::CheckBox check1;
                GUIKIT::CheckBox check2;
                GUIKIT::CheckBox check3;
                Flags();
            };

            struct Selected : GUIKIT::HorizontalLayout {
                GUIKIT::Label label;
                GUIKIT::CheckBox checks[4];

                Selected();
            };

            struct Fifo : GUIKIT::FramedHorizontalLayout {
                GUIKIT::ImageView imgIn;
                GUIKIT::LineEdit edit1;
                GUIKIT::LineEdit edit2;
                GUIKIT::LineEdit edit3;
                GUIKIT::ImageView imgOut;

                Fifo();
            };

            Entry entry1;
            Entry entry2;
            Entry entry3;

            Selected selected;

            Flags flags1;
            Flags flags2;
            Fifo fifo;

            Fdc();
        } fdc;

        Paula();
    } *paula = nullptr;

    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Paula; }
};
