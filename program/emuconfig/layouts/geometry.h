
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

        Type1();
    } type1;

    struct Type2 : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox cropSemiAuto;
        GUIKIT::RadioBox cropFree;

        Type2();
    } type2;

    struct Hotkey : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::CheckBox cropOff;
        GUIKIT::CheckBox cropMonitor;
        GUIKIT::CheckBox cropAuto;
        GUIKIT::CheckBox cropSemiAuto;
        GUIKIT::CheckBox cropFree;

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

    GeometryLayout(TabWindow* tabWindow);
};
