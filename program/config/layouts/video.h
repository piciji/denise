
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

struct VideoFrameAdjustLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::CheckBox overrideExactFrequency;
    GUIKIT::Label pal;
    GUIKIT::LineEdit palFrequency;
    GUIKIT::Label ntsc;
    GUIKIT::LineEdit ntscFrequency;
    GUIKIT::Label hint;
    
    VideoFrameAdjustLayout();
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

struct VideoFpsLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout updateDelay;

    struct DecimalPoint : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox Zero;
        GUIKIT::RadioBox One;
        GUIKIT::RadioBox Two;
        GUIKIT::RadioBox Three;

        DecimalPoint();
    } decimalPoint;

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
    VideoFrameAdjustLayout videoFrameAdjust;
    CrtEmulationLayout crtEmulation;
	VideoGeometryLayout videoGeometry;
    VideoResolutionLayout videoResolution;
    VideoFpsLayout videoFps;

    VideoSettingsLayout videoSettingsLayout;
    PathsLayout paths;

    DriverLayout driverLayout;
    GUIKIT::HorizontalLayout hLayout;

    auto translate() -> void;
    auto updateFrequencyLayout() -> void;

    VideoLayout();
};
