
struct InScreenTextLayout : GUIKIT::FramedVerticalLayout {
    GUIKIT::RadioBox option1;
    GUIKIT::RadioBox option2;
    GUIKIT::RadioBox option3;

    InScreenTextLayout();
};

struct CrtEmulationLayout : GUIKIT::FramedVerticalLayout {
    GUIKIT::CheckBox threadMode;
    GUIKIT::CheckBox shaderInputPrecision;
    
    CrtEmulationLayout();
};

struct VideoGeometryLayout : GUIKIT::FramedVerticalLayout {
    GUIKIT::CheckBox aspectCorrectResizing;

    struct Dimension : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit width;
        GUIKIT::LineEdit height;

        Dimension();
    } dimension;

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::Button refresh;
        GUIKIT::Button apply;

        Control();
    } control;

    VideoGeometryLayout();
};

struct PathsLayout : GUIKIT::FramedVerticalLayout {

    struct Block : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit edit;
        GUIKIT::Button empty;
        GUIKIT::Button select;

        Block();
    } shader;

    PathsLayout();
};

struct VideoSettingsLayout : GUIKIT::FramedHorizontalLayout {
    
    GUIKIT::CheckBox exclusiveFullscreen;
    GUIKIT::CheckBox hardSync;
    GUIKIT::CheckBox threadedRenderer;
    
    VideoSettingsLayout();
};

struct VideoFpsLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout updateDelay;

    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::Label labelDecimalPlace;
        GUIKIT::RadioBox Zero;
        GUIKIT::RadioBox One;
        GUIKIT::RadioBox Two;
        GUIKIT::RadioBox Three;

        Options();
    } options;

    VideoFpsLayout();
};

struct VideoResolutionLayout : GUIKIT::FramedHorizontalLayout {

    GUIKIT::CheckBox active;
    GUIKIT::ComboButton display;
    GUIKIT::ComboButton displaySettings;

    VideoResolutionLayout();
};

struct VideoLayout : GUIKIT::VerticalLayout {
    InScreenTextLayout screenTextLayout;
    CrtEmulationLayout crtEmulation;
	VideoGeometryLayout videoGeometry;
    VideoResolutionLayout videoResolution;
    VideoFpsLayout videoFps;

    VideoSettingsLayout videoSettingsLayout;
    PathsLayout paths;

    DriverLayout driverLayout;
    GUIKIT::HorizontalLayout hLayout;

    auto translate() -> void;
    auto updateDriverPropsVisibility() -> void;

    VideoLayout();
};
