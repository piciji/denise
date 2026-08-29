
#pragma once

#include "../../../guikit/api.h"
#include "../../program.h"
#include "../../config/slider.h"
#include "model.h"

namespace EmuConfigView {

struct InputSamplingLayout : GUIKIT::FramedVerticalLayout {

    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox staticMode;
        GUIKIT::RadioBox restrictedDynamicMode;
        GUIKIT::RadioBox dynamicMode;

        Options();
    } options;

    SliderLayout control;

    InputSamplingLayout();
};

struct RunAheadLayout : GUIKIT::FramedVerticalLayout {
    SimpleSliderLayout control;
    
    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox performanceMode;
        GUIKIT::CheckBox disableOnPower;
        GUIKIT::CheckBox preventDynamic;
        
        Options();
    } options;
    
    RunAheadLayout();
};

struct AutostartLayout : GUIKIT::FramedVerticalLayout {

    struct AutoWarp : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox off;
        GUIKIT::RadioBox normal;
        GUIKIT::RadioBox aggressive;
        GUIKIT::RadioBox fastforward;
        GUIKIT::CheckBox diskFirstFile;
        GUIKIT::CheckBox tapeFirstFile;
        GUIKIT::CheckBox disableWarpWhenInput;

        AutoWarp(Emulator::Interface* emulator);
    } autoWarp;

    GUIKIT::CheckBox manuellOverAutowarp;

    struct StartWrapper : GUIKIT::HorizontalLayout {
        struct Start : GUIKIT::VerticalLayout {
            GUIKIT::CheckBox diskTrapsOnDblClick;
            GUIKIT::CheckBox tapeTrapsOnDblClick;

            Start();
        } start;

        struct Option : GUIKIT::VerticalLayout {
            struct DiskOptions : GUIKIT::HorizontalLayout {
                GUIKIT::CheckBox loadWithColumn;
                GUIKIT::CheckBox speederTraps;

                DiskOptions();
            } diskOptions;
            GUIKIT::CheckBox tapeWithStandardKernal;

            Option();
        } option;

        StartWrapper();
    };

    StartWrapper* startWrapper = nullptr;

    struct DragnDrop : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox power;
        GUIKIT::CheckBox captureMouse;

        DragnDrop(Emulator::Interface* emulator);
    } dragnDrop;

    AutostartLayout(Emulator::Interface* emulator);
};

struct MiscLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    InputSamplingLayout inputSamplingLayout;
    RunAheadLayout runAheadLayout;
    AutostartLayout autostartLayout;
    
    auto translate() -> void;
    auto setRunAheadPerformance(bool state) -> void;
    auto setRunAhead(unsigned pos, bool force = true) -> void;
    auto loadSettings() -> void;
    auto initAutowarp(Emulator::Interface::MediaGroup* forGroup = nullptr) -> void;
    
    MiscLayout(TabWindow* tabWindow);
};

}
