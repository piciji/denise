
#pragma once

#include "debugger.h"

struct SerialDebugger : Debugger {
    explicit SerialDebugger( Emulator::Interface* emulator );

    struct Serial : GUIKIT::VerticalLayout {

        struct Top : GUIKIT::HorizontalLayout {

            struct Uart : GUIKIT::FramedVerticalLayout {
                struct Transmit : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;
                    GUIKIT::ImageView imageView;
                    GUIKIT::Label labelR;
                    GUIKIT::LineEdit editR;
                    GUIKIT::Label labelB;
                    GUIKIT::LineEdit editB;

                    Transmit();
                } transmit;

                struct Receive : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;
                    GUIKIT::ImageView imageView;
                    GUIKIT::Label labelR;
                    GUIKIT::LineEdit editR;
                    GUIKIT::RadioBox radio8Bit;
                    GUIKIT::RadioBox radio9Bit;

                    Receive();
                } receive;

                struct SerdatR : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;

                    GUIKIT::CheckBox overrun;
                    GUIKIT::CheckBox rbf;
                    GUIKIT::CheckBox tbe;
                    GUIKIT::CheckBox tsre;
                    GUIKIT::CheckBox rxd;

                    SerdatR();
                } serdatR;

                Uart();
            } uart;

            struct Pins : GUIKIT::FramedVerticalLayout {
                struct Flags : GUIKIT::HorizontalLayout {
                    GUIKIT::CheckBox check1;
                    GUIKIT::CheckBox check2;
                    GUIKIT::CheckBox check3;
                    GUIKIT::CheckBox check4;
                    Flags();
                } line1, line2;

                Pins();
            } pins;

            Top();
        } top;

        struct Bottom : GUIKIT::HorizontalLayout {
            struct Data : GUIKIT::FramedVerticalLayout {
                GUIKIT::MultilineEdit edit;

                Data();
            } outgoing, incoming;

            Bottom();
        } bottom;

        Serial();
    } *serial = nullptr;

    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;

    auto getTheme() -> DebuggerTheme override { return DebuggerTheme::Serial; }
};
