
struct VideoSettingsLayout : GUIKIT::FramedHorizontalLayout {
    
    GUIKIT::CheckBox exclusiveFullscreen;
    GUIKIT::CheckBox hardSync;
    GUIKIT::Label threadedRenderer;
    GUIKIT::CheckBox trOn;
    GUIKIT::CheckBox trAuto;
    
    VideoSettingsLayout();
};

struct VideoLayout : GUIKIT::VerticalLayout {
    VideoSettingsLayout videoSettingsLayout;

    DriverLayout driverLayout;

    auto translate() -> void;
    auto updateDriverPropsVisibility() -> void;

    VideoLayout();
};
