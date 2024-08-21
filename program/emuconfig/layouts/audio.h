
#include "../../../guikit/api.h"

struct AudioRecordLayout : GUIKIT::FramedVerticalLayout {
    
    struct Location : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit path;
        GUIKIT::Button select;
        
        Location();
    } location;
    
    struct Duration : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox useTimeLimit;
        SliderLayout minutesSlider;
        SliderLayout secondsSlider;
        
        GUIKIT::CheckButton record;
        
        Duration();
    } duration;
    
    AudioRecordLayout();
};

struct AudioDriveLayout : GUIKIT::FramedVerticalLayout {
    struct Selection : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::ComboButton combo;
        GUIKIT::Widget spacer;
        GUIKIT::Button reload;
        Selection();
    };

    SliderLayout floppyVolume;
    Selection floppySelection;

    SliderLayout tapeVolume;
    Selection tapeSelection;

    SliderLayout tapeNoiseVolume;

    AudioDriveLayout(Emulator::Interface* emulator);
};

struct BassControlLayout : GUIKIT::FramedVerticalLayout {
    
    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SliderLayout frequency;                
        
        TopLayout();
        
    } top;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        SliderLayout gain;
        SliderLayout reduceClipping;
        
        BottomLayout();
        
    } bottom;
    
    BassControlLayout();
};

struct ReverbControlLayout : GUIKIT::FramedVerticalLayout {
    
    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SliderLayout dryTime;                
        SliderLayout wetTime;
        
        TopLayout();
        
    } top;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        SliderLayout damping;
        SliderLayout roomWidth;
        SliderLayout roomSize;        
        
        BottomLayout();
        
    } bottom;
    
    ReverbControlLayout();
};

struct PanningControlLayout : GUIKIT::FramedVerticalLayout {

    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SliderLayout separation;

        TopLayout();
    } top;

    struct MiddleLayout : GUIKIT::HorizontalLayout {
        GUIKIT::Label leftChannel;
        SliderLayout leftMix;
        SliderLayout rightMix;

        MiddleLayout();
    } middle;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        GUIKIT::Label rightChannel;
        SliderLayout leftMix;
        SliderLayout rightMix;
        
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

struct AudioLayout : GUIKIT::HorizontalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::Image recordAudioImage;
    GUIKIT::Image sineImage;
    GUIKIT::Image processorImage;
    GUIKIT::Image driveImage;
    
    GUIKIT::FramedVerticalLayout moduleFrame;
    GUIKIT::ListView moduleList;

    GUIKIT::SwitchLayout moduleSwitch;
    
    ModelLayout settingsLayout;
    
    GUIKIT::VerticalLayout dspFrame;
    BassControlLayout bass;
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
};
