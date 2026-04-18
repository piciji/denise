
#pragma once

#include "debugger.h"

struct AgnusDebugger : Debugger {
    explicit AgnusDebugger( Emulator::Interface* emulator );

    struct Agnus : GUIKIT::HorizontalLayout {

        struct Entry : GUIKIT::HorizontalLayout {
            GUIKIT::CheckBox check;
            GUIKIT::Label label;
            GUIKIT::LineEdit edit;

            Entry(bool useCheck = true);
        };

        struct ContainerBplRef : GUIKIT::VerticalLayout {
            struct Bpl : GUIKIT::FramedVerticalLayout  {
                std::vector<Entry*> entries;
                Entry mod1;
                Entry mod2;
                Bpl();
            } bpl;

            struct Ref : GUIKIT::FramedVerticalLayout {
                Entry entry;
                Ref();
            } ref;

            ContainerBplRef();
        } containerBplRef;

        struct ContainerBltCop : GUIKIT::VerticalLayout {
            struct Blt : GUIKIT::FramedVerticalLayout  {
                std::vector<Entry*> entries;
                std::vector<Entry*> mods;
                Blt();
            } blt;

            struct Cop : GUIKIT::FramedVerticalLayout {
                Entry entry;
                Cop();
            } cop;

            ContainerBltCop();
        } containerBltCop;

        struct ContainerSprDsk : GUIKIT::VerticalLayout {
            struct Spr : GUIKIT::FramedVerticalLayout  {
                std::vector<Entry*> entries;
                Spr();
            } spr;

            struct Dsk : GUIKIT::FramedVerticalLayout {
                Entry entry;
                Dsk();
            } dsk;

            ContainerSprDsk();
        } containerSprDsk;

        struct ContainerAudReg : GUIKIT::VerticalLayout {
            struct Aud : GUIKIT::FramedVerticalLayout {
                struct Entry : GUIKIT::HorizontalLayout {
                    GUIKIT::CheckBox check;
                    GUIKIT::LineEdit edit;
                    GUIKIT::Label label;
                    GUIKIT::LineEdit editLatch;
                    GUIKIT::Label labelLatch;

                    Entry();
                };
                std::vector<Entry*> entries;

                Aud();
            } aud;

            struct Reg : GUIKIT::FramedVerticalLayout {
                struct Entry : GUIKIT::HorizontalLayout {
                    GUIKIT::Label label;
                    GUIKIT::LineEdit edit;
                    GUIKIT::Label labelR;
                    GUIKIT::LineEdit editR;

                    Entry();
                } line1, line2, line3, line4;
                GUIKIT::CheckBox bltPri;

                Reg();
            } reg;

            ContainerAudReg();
        } containerAudReg;

        Agnus();
    } *agnus = nullptr;

    auto saveIdent() -> std::string override;
    auto titleIdent() -> std::string override;
    auto buildTheme() -> GUIKIT::Layout* override;
    auto translateTheme() -> void override;
    auto updateTheme() -> void override;
    auto initTheme() -> void override;
    auto closeTheme() -> void override;
};
