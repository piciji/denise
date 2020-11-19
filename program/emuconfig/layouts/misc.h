
struct RunAheadLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout control;
    
    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox performanceMode;
        GUIKIT::CheckBox disableOnPower;
        
        Options();
    } options;
    
    RunAheadLayout();
};

struct MiscLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    RunAheadLayout runAheadLayout;    
    
    auto translate() -> void;
    auto setRunAheadPerformance(bool state) -> void;
    auto setRunAhead(unsigned pos, bool force = true) -> void;
    auto loadSettings() -> void;
    
    MiscLayout(TabWindow* tabWindow);
};