
struct InScreenTextLayout : GUIKIT::FramedVerticalLayout {
    GUIKIT::RadioBox option1;
    GUIKIT::RadioBox option2;
    GUIKIT::RadioBox option3;

    InScreenTextLayout();
};

struct CrtEmulation : GUIKIT::FramedVerticalLayout {
    GUIKIT::CheckBox threadMode;
    GUIKIT::CheckBox shaderInputPrecision;
    
    CrtEmulation();
};

struct VideoFrameAdjust : GUIKIT::FramedHorizontalLayout {
    GUIKIT::CheckBox overrideExactFrequency;
    GUIKIT::Label pal;
    GUIKIT::LineEdit palFrequency;    
    GUIKIT::Label ntsc;
    GUIKIT::LineEdit ntscFrequency;    
    
    VideoFrameAdjust();
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

struct VideoLayout : GUIKIT::VerticalLayout {
    InScreenTextLayout screenTextLayout;
    VideoFrameAdjust videoFrameAdjust;
    CrtEmulation crtEmulation;

    VideoSettingsLayout videoSettingsLayout;
    PathsLayout paths;

    DriverLayout driverLayout;
    GUIKIT::HorizontalLayout hLayout;

    auto translate() -> void;
    auto updateFrequencyLayout() -> void;

    VideoLayout();
};
