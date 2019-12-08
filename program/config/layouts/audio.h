
struct AudioControlLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label frequencyLabel;
    GUIKIT::ComboButton frequencyCombo;
    GUIKIT::CheckBox reverb;    
    GUIKIT::Label maxRateLabel;
    GUIKIT::LineEdit maxRateEdit;
    
    AudioControlLayout();
};

struct AudioLayout : GUIKIT::VerticalLayout {

    AudioControlLayout control;
    SliderLayout latency;    
    SliderLayout volume;                
    
	GUIKIT::FramedVerticalLayout frame;
    
    DriverLayout driverLayout;

    auto translate() -> void;
    auto updateLatencySlider() -> void;

    AudioLayout();
};
