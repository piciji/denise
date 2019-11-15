
struct AudioControlLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label frequencyLabel;
    GUIKIT::ComboButton frequencyCombo;
    GUIKIT::CheckBox reverb;    
    GUIKIT::Label maxRateLabel;
    GUIKIT::LineEdit maxRateEdit;
    
    AudioControlLayout();
};

struct AudioSliderLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label name;
    GUIKIT::Label value;
    GUIKIT::HorizontalSlider slider;
    GUIKIT::Button button;

    AudioSliderLayout();
};

struct AudioLayout : GUIKIT::VerticalLayout {

    AudioControlLayout control;
    AudioSliderLayout latency;    
    AudioSliderLayout volume;                
    
	GUIKIT::FramedVerticalLayout frame;
    
    DriverLayout driverLayout;

    auto translate() -> void;
    auto updateLatencySlider() -> void;

    AudioLayout();
};
