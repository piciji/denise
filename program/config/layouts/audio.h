
struct AudioControlLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label frequencyLabel;
    GUIKIT::ComboButton frequencyCombo;
    GUIKIT::CheckBox priorityCheckbox;
    GUIKIT::Label maxRateLabel;
    GUIKIT::LineEdit maxRateEdit; 
    DriverLayout driverLayout;
    
    AudioControlLayout();
};

struct AudioLayout : GUIKIT::VerticalLayout {

    AudioControlLayout control;
    SliderLayout latency;    
    SliderLayout volume;                
    
	GUIKIT::FramedVerticalLayout frame;    

    auto translate() -> void;
    auto updateLatencySlider() -> void;

    AudioLayout();
};
