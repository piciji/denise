
struct DriverLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Widget spacer;
    GUIKIT::Label name;
    GUIKIT::ComboButton combo;

    DriverLayout() {
        append(spacer, {~0u, 0u});
        append(name, {0u, 0u}, 5);
        append(combo, {0u, 0u});

        setAlignment(0.5);
    }
};

struct VideoDriverLayout : GUIKIT::FramedVerticalLayout {

    struct Top : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox exclusiveFullscreen;
        GUIKIT::CheckBox hardSync;
        DriverLayout driver;

        Top();
    } top;

    struct Bottom : GUIKIT::HorizontalLayout {
        GUIKIT::Label threadedRenderer;
        GUIKIT::CheckBox trOn;
        GUIKIT::CheckBox trAuto;

        Bottom();
    } bottom;

    VideoDriverLayout();
};

struct AudioDriverLayout : GUIKIT::FramedVerticalLayout {

    struct Top : GUIKIT::HorizontalLayout {
        GUIKIT::Label frequencyLabel;
        GUIKIT::ComboButton frequencyCombo;
        GUIKIT::CheckBox priorityCheckbox;
        GUIKIT::Label maxRateLabel;
        GUIKIT::LineEdit maxRateEdit;
        DriverLayout driver;

        Top();
    } top;

    struct Bottom : GUIKIT::HorizontalLayout {
        SliderLayout latency;

        Bottom();
    } bottom;

    AudioDriverLayout();
};

struct InputDriverLayout : GUIKIT::FramedVerticalLayout {

    struct Top : GUIKIT::HorizontalLayout {
        DriverLayout driver;

        Top();
    } top;

    InputDriverLayout();
};

struct DriversLayout : GUIKIT::VerticalLayout {
    VideoDriverLayout vdl;
    AudioDriverLayout adl;
    InputDriverLayout idl;

    auto translate() -> void;
    auto updateDriverPropsVisibility() -> void;
    auto updateLatencySlider() -> void;

    DriversLayout();
};
