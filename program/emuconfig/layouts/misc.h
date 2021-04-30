
struct RunAheadLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout control;
    
    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox performanceMode;
        GUIKIT::CheckBox disableOnPower;
        
        Options();
    } options;
    
    RunAheadLayout();
};

struct WarpLayout : GUIKIT::FramedHorizontalLayout {

    GUIKIT::RadioBox off;
    GUIKIT::RadioBox normal;
    GUIKIT::RadioBox aggressive;
    GUIKIT::CheckBox diskFirstFile;
    GUIKIT::CheckBox tapeFirstFile;

    WarpLayout();
};

struct MiscLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    RunAheadLayout runAheadLayout;
    WarpLayout warpLayout;
    
    auto translate() -> void;
    auto setRunAheadPerformance(bool state) -> void;
    auto setRunAhead(unsigned pos, bool force = true) -> void;
    auto loadSettings() -> void;
    
    MiscLayout(TabWindow* tabWindow);
};