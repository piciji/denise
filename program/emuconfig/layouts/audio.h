
#pragma once

#include "../../../guikit/api.h"
#include "../../program.h"
#include "../../config/slider.h"
#include "model.h"

namespace EmuConfigView {

struct TabWindow;

struct AudioRecordLayout : GUIKIT::FramedVerticalLayout {
    
    struct Location : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit pathEdit;
        GUIKIT::Button standard;
        GUIKIT::Button select;
        
        Location();
    } location;

    struct Type : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox mp3;
        GUIKIT::RadioBox wav;

        Type();
    } type;
    
    struct Duration : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox useTimeLimit;
        SimpleSliderLayout minutesSlider;
        SimpleSliderLayout secondsSlider;
        
        GUIKIT::CheckButton record;
        
        Duration();
    } duration;
    
    AudioRecordLayout();
};

struct AudioDriveLayout : GUIKIT::FramedVerticalLayout {
    struct TapeSelection : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::ComboButton combo;
        GUIKIT::Widget spacer;
        GUIKIT::Button reload;
        TapeSelection();
    } tapeSelection;

    struct FloppySelection : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::ComboButton combo;
        GUIKIT::Label labelExt;
        GUIKIT::ComboButton comboExt;
        GUIKIT::Widget spacer;
        GUIKIT::Button reload;
        FloppySelection();
    } floppySelection;

    SliderLayout floppyVolume;
    SliderLayout floppyVolumeExt;
    SliderLayout tapeVolume;
    SliderLayout tapeNoiseVolume;

    AudioDriveLayout(Emulator::Interface* emulator);
};

struct BassControlLayout : GUIKIT::FramedVerticalLayout {
    
    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SliderLayout frequency;                
        GUIKIT::Button reset;
        TopLayout();
        
    } top;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        SimpleSliderLayout gain;
        SimpleSliderLayout reduceClipping;
        
        BottomLayout();
        
    } bottom;
    
    BassControlLayout();
};

struct EchoControlLayout : GUIKIT::FramedVerticalLayout {

    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        GUIKIT::Button echoReverb;
        SimpleSliderLayout amp;
        GUIKIT::Button reset;

        TopLayout();
    } top;

    struct BottomLayout : GUIKIT::HorizontalLayout {
        SliderLayout delay;
        SimpleSliderLayout feedback;

        BottomLayout();

    } bottom;

    EchoControlLayout();
};

struct ReverbControlLayout : GUIKIT::FramedVerticalLayout {
    
    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SimpleSliderLayout dryTime;
        SimpleSliderLayout wetTime;
        GUIKIT::Button reset;
        TopLayout();
        
    } top;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        SimpleSliderLayout damping;
        SimpleSliderLayout roomWidth;
        SimpleSliderLayout roomSize;
        
        BottomLayout();
        
    } bottom;
    
    ReverbControlLayout();
};

struct PanningControlLayout : GUIKIT::FramedVerticalLayout {

    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SliderLayout separation;
        GUIKIT::Button reset;
        TopLayout();
    } top;

    struct MiddleLayout : GUIKIT::HorizontalLayout {
        GUIKIT::Label leftChannel;
        SimpleSliderLayout leftMix;
        SimpleSliderLayout rightMix;

        MiddleLayout();
    } middle;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        GUIKIT::Label rightChannel;
        SimpleSliderLayout leftMix;
        SimpleSliderLayout rightMix;
        
        BottomLayout();
    } bottom;
    
    PanningControlLayout();
};

struct VolumeControlLayout : GUIKIT::HorizontalLayout {
    GUIKIT::VerticalSlider volumeSlider;

    struct Info : GUIKIT::VerticalLayout {
        GUIKIT::Label label;
        GUIKIT::Label value;

        Info();
    } info;

    VolumeControlLayout();
};

struct PicoWindow : GUIKIT::Window {
    GUIKIT::Widget spacer;
    GUIKIT::HorizontalLayout layout;
};

struct AudioModelLayout : ModelLayout {

    struct ControlLayout : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::CheckBox firstAll;
        GUIKIT::CheckBox secondAll;
        GUIKIT::Widget spacer;
        GUIKIT::Button button;

        ControlLayout();
    } controlLayout;

    auto lineWillAppend( unsigned pos ) -> void override;
    auto updateVisibillity( ) -> void override;
    auto updated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;

    auto updateBiasVisibillity() -> void;
    auto updateExtraSidVisibillity() -> void;

    auto getIdent( Emulator::Interface::Model* model, std::string& tooltip ) -> std::string override;
};

struct PicoModelLayout : ModelLayout {
    auto updated( Line::Block* block, Emulator::Interface::Model* model ) -> void override;
    auto updateVisibillity( ) -> void override;
};

struct AudioLayout : GUIKIT::HorizontalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::Image recordAudioImage;
    GUIKIT::Image sineImage;
    GUIKIT::Image processorImage;
    GUIKIT::Image driveImage;
    GUIKIT::Image resetImage;
    
    GUIKIT::FramedVerticalLayout moduleFrame;
    GUIKIT::ListView moduleList;

    GUIKIT::SwitchLayout moduleSwitch;
    
    AudioModelLayout settingsLayout;
    PicoModelLayout* usbSidPicoLayout = nullptr;
    PicoWindow* picoWindow = nullptr;
    
    GUIKIT::VerticalLayout dspFrame;
    BassControlLayout bass;
    EchoControlLayout echo;
    ReverbControlLayout reverb;
    PanningControlLayout panning;
    AudioDriveLayout* driveLayout;
    VolumeControlLayout volumeLayout;
    
    AudioRecordLayout audioRecord;
    
    AudioLayout(TabWindow* tabWindow);
    
    auto translate() -> void;
    
    auto loadSettings() -> void;
    
    auto initDsp(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void;
    
    auto setDspEvent(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void;
    
    auto updateVisibility() -> void;
    
    auto stopRecord() -> void;
    
    auto toggleRecord() -> void;

    auto updateFloppyProfileList() -> void;
    auto updateTapeProfileList() -> void;

    auto initSeparation() -> void;
    auto setSeparation() -> void;

    auto updateVolumeSlider() -> void;
    auto updateRecordingPath() -> void;

    auto selectViewAudioRecord() -> void;
};

}
