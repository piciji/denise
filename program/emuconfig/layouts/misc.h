
struct RunAheadLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout control;
    
    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox performanceMode;
        GUIKIT::CheckBox disableOnPower;
        
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
        GUIKIT::CheckBox diskFirstFile;
        GUIKIT::CheckBox tapeFirstFile;

        AutoWarp();
    } autoWarp;

    GUIKIT::CheckBox tapeWithStandardKernal;

    AutostartLayout();
};

struct MiscLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    RunAheadLayout runAheadLayout;
    AutostartLayout autostartLayout;
    
    auto translate() -> void;
    auto setRunAheadPerformance(bool state) -> void;
    auto setRunAhead(unsigned pos, bool force = true) -> void;
    auto loadSettings() -> void;
    
    MiscLayout(TabWindow* tabWindow);
};