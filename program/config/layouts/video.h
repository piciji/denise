
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
    GUIKIT::CheckBox aspectCorrect;
    GUIKIT::CheckBox integerScaling;
    
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
    
    VideoSettingsLayout();
};

struct VideoSpeedLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::Label labelSpeed;
    GUIKIT::ComboButton profile;
    GUIKIT::LineEdit speed;
    GUIKIT::RadioBox fps;
    GUIKIT::RadioBox percent;

    VideoSpeedLayout();
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
    VideoSpeedLayout videoSpeed;

    VideoSettingsLayout videoSettingsLayout;
    PathsLayout paths;

    DriverLayout driverLayout;
    GUIKIT::HorizontalLayout hLayout;

    auto translate() -> void;

    VideoLayout();
};
