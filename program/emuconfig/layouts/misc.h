
struct SpeedLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::Label label;
    GUIKIT::RadioBox fps;
    GUIKIT::RadioBox percent;
    GUIKIT::LineEdit speed;
    GUIKIT::Button apply;

    SpeedLayout();
};

struct JitLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::CheckBox active;
    SliderLayout control;

    JitLayout();
};

struct RunAheadLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout control;
    
    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox performanceMode;
        GUIKIT::CheckBox disableOnPower;
        GUIKIT::CheckBox preventJit;
        
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

    struct DiskStart : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox diskTrapsOnDblClick;
        GUIKIT::CheckBox loadWithColumn;

        DiskStart();
    } diskStart;

    struct TapeStart : GUIKIT::HorizontalLayout {
		GUIKIT::CheckBox tapeTrapsOnDblClick;		
        GUIKIT::CheckBox tapeWithStandardKernal;        

        TapeStart();
    } tapeStart;

    AutostartLayout();
};

struct MiscLayout : GUIKIT::VerticalLayout {
    
    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    SpeedLayout speedLayout;
    JitLayout jitLayout;
    RunAheadLayout runAheadLayout;
    AutostartLayout* autostartLayout = nullptr;
    
    auto translate() -> void;
    auto setRunAheadPerformance(bool state) -> void;
    auto setRunAhead(unsigned pos, bool force = true) -> void;
    auto loadSettings() -> void;
    
    MiscLayout(TabWindow* tabWindow);
};