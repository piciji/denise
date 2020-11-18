
struct RunAheadLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout control;
    
    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox performanceMode;
        GUIKIT::CheckBox disableOnPower;
        
        Options();
    } options;
    
    RunAheadLayout();
};

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

struct MiscLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    RunAheadLayout runAheadLayout;
    AudioRecordLayout audioRecordLayout;
    
    auto translate() -> void;
    auto setRunAheadPerformance(bool state) -> void;
    auto setRunAhead(unsigned pos, bool force = true) -> void;
    auto stopRecord() -> void;
    auto toggleRecord() -> void;
    auto loadSettings() -> void;
    
    MiscLayout(TabWindow* tabWindow);
};