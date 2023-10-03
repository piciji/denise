
struct MonitorResolutionLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::CheckBox active;
    GUIKIT::ComboButton display;
    GUIKIT::ComboButton displaySettings;

    MonitorResolutionLayout();
};

struct CropLayout : GUIKIT::FramedVerticalLayout {

    struct Type1 : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox cropOff;
        GUIKIT::RadioBox cropMonitor;
        GUIKIT::RadioBox cropAuto;
        GUIKIT::RadioBox cropSemiAuto;

        Type1();
    } type1;

    struct Type2 : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox cropFree1;
        GUIKIT::RadioBox cropFree2;
        GUIKIT::RadioBox cropFree3;

        Type2();
    } type2;

    struct Type3 : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox cropFree4;
        GUIKIT::RadioBox cropFree5;
        GUIKIT::RadioBox cropFree6;

        Type3();
    } type3;

    struct Hotkey : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::CheckBox cropOff;
        GUIKIT::CheckBox cropMonitor;
        GUIKIT::CheckBox cropAuto;
        GUIKIT::CheckBox cropSemiAuto;
        GUIKIT::CheckBox cropFree1;
        GUIKIT::CheckBox cropFree2;
        GUIKIT::CheckBox cropFree3;
        GUIKIT::CheckBox cropFree4;
        GUIKIT::CheckBox cropFree5;
        GUIKIT::CheckBox cropFree6;

        Hotkey();
    } hotkey;

    GUIKIT::CheckBox aspectCorrect;
    SliderLayout cropLeft;
    SliderLayout cropRight;
    SliderLayout cropTop;
    SliderLayout cropBottom;

    CropLayout();
};

struct RatioLayout : GUIKIT::FramedHorizontalLayout {

    GUIKIT::Label label;
    GUIKIT::RadioBox window;
    GUIKIT::RadioBox tv;
    GUIKIT::RadioBox native;
    GUIKIT::CheckBox integerScaling;

    RatioLayout();
};

struct GeometryLayout : GUIKIT::VerticalLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;

    CropLayout cropLayout;
    RatioLayout ratioLayout;
    MonitorResolutionLayout monitorResolutionLayout;

    auto translate() -> void;
	auto updateVisibillity() -> void;
    auto loadSettings() -> void;
    auto updateCrop(std::string property, unsigned value) -> void;
    auto updateBorderHotkeyUsage(unsigned bit, bool checked) -> void;
    auto updateBorderSlider() -> void;

    GeometryLayout(TabWindow* tabWindow);
};
