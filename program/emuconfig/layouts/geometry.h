
struct MonitorResolutionLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::CheckBox active;
    GUIKIT::ComboButton display;
    GUIKIT::ComboButton displaySettings;

    MonitorResolutionLayout();
};

struct RotationLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::Label rotation;
    GUIKIT::RadioBox degree0;
    GUIKIT::RadioBox degree90;
    GUIKIT::RadioBox degree180;
    GUIKIT::RadioBox degree270;

    RotationLayout();
};

struct CropLayout : GUIKIT::FramedVerticalLayout {

    struct Type1 : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox cropOff;
        GUIKIT::RadioBox cropMonitor;
        GUIKIT::RadioBox cropAutoAspect;
        GUIKIT::RadioBox cropAuto;

        Type1();
    } type1;

    struct Type2 : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox cropAllSidesAspect;
        GUIKIT::RadioBox cropAllSides;

        Type2();
    } type2;

    struct Type3 : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox cropFree1;
        GUIKIT::RadioBox cropFree2;
        GUIKIT::RadioBox cropFree3;

        Type3();
    } type3;

    struct Type4 : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox cropFree4;
        GUIKIT::RadioBox cropFree5;
        GUIKIT::RadioBox cropFree6;

        Type4();
    } type4;

    struct Hotkey : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        std::vector<GUIKIT::CheckBox*> boxes;
        GUIKIT::Widget spacer;
        GUIKIT::Button reset;
        Hotkey();
    } hotkey;

    SliderLayout cropLeft;
    SliderLayout cropRight;
    SliderLayout cropTop;
    SliderLayout cropBottom;

    CropLayout();
};

struct RatioLayout : GUIKIT::FramedVerticalLayout {

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox window;
        GUIKIT::RadioBox tv;
        GUIKIT::RadioBox native;
        GUIKIT::RadioBox nativeFree;
        GUIKIT::CheckBox integerScaling;
        Control();
    } control;

    struct Hotkey : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        std::vector<GUIKIT::CheckBox*> boxes;

        Hotkey();
    } hotkey;

    struct Dimension : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit width;
        GUIKIT::LineEdit height;

        GUIKIT::Button refresh;
        GUIKIT::Button apply;

        GUIKIT::Button cropWindow;
        GUIKIT::Widget spacer;
        GUIKIT::CheckBox aspectCorrectResizing;

        Dimension();
    } dimension;

    RatioLayout();
};

struct GeometryLayout : GUIKIT::VerticalLayout {
    TabWindow* tabWindow;
    Emulator::Interface* emulator;

    CropLayout cropLayout;
    RatioLayout ratioLayout;
    MonitorResolutionLayout monitorResolutionLayout;
    RotationLayout rotationLayout;

    auto translate() -> void;
	auto updateVisibillity() -> void;
    auto loadSettings() -> void;
    auto updateCrop(std::string property, unsigned value = 0) -> void;
    auto updateBorderHotkeyUsage(unsigned bit, bool checked) -> void;
    auto updateScaleHotkeyUsage(unsigned bit, bool checked) -> void;
    auto updateBorderSlider() -> void;
    auto setRotation(DRIVER::Rotation rotation) -> void;

    GeometryLayout(TabWindow* tabWindow);
};
