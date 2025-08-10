
struct AudioRecordLayout : GUIKIT::FramedVerticalLayout {
    
    struct Location : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit pathEdit;
        GUIKIT::Button standard;
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
        SliderLayout gain;
        SliderLayout reduceClipping;
        
        BottomLayout();
        
    } bottom;
    
    BassControlLayout();
};

struct EchoControlLayout : GUIKIT::FramedVerticalLayout {

    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        GUIKIT::Button echoReverb;
        SliderLayout amp;
        GUIKIT::Button reset;

        TopLayout();
    } top;

    struct BottomLayout : GUIKIT::HorizontalLayout {
        SliderLayout delay;
        SliderLayout feedback;

        BottomLayout();

    } bottom;

    EchoControlLayout();
};

struct ReverbControlLayout : GUIKIT::FramedVerticalLayout {
    
    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SliderLayout dryTime;                
        SliderLayout wetTime;
        GUIKIT::Button reset;
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
        GUIKIT::Button reset;
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
    GUIKIT::Image resetImage;
    
    GUIKIT::FramedVerticalLayout moduleFrame;
    GUIKIT::ListView moduleList;

    GUIKIT::SwitchLayout moduleSwitch;
    
    ModelLayout settingsLayout;
    ModelLayout* usbSidPicoLayout = nullptr;
    
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
