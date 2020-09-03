
struct AudioControlLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label frequencyLabel;
    GUIKIT::ComboButton frequencyCombo;
    GUIKIT::CheckBox priorityCheckbox;
    GUIKIT::Label maxRateLabel;
    GUIKIT::LineEdit maxRateEdit; 
    DriverLayout driverLayout;
    
    AudioControlLayout();
};

struct BassControlLayout : GUIKIT::FramedVerticalLayout {
    
    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SliderLayout frequency;                
        
        TopLayout();
        
    } top;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        SliderLayout gain;
        SliderLayout reduceClipping;
        
        BottomLayout();
        
    } bottom;
    
    BassControlLayout();
};

struct ReverbControlLayout : GUIKIT::FramedVerticalLayout {
    
    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        SliderLayout dryTime;                
        SliderLayout wetTime;
        
        TopLayout();
        
    } top;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        SliderLayout damping;
        SliderLayout roomWidth;
        SliderLayout roomSize;        
        
        BottomLayout();
        
    } bottom;
    
    ReverbControlLayout();
};

struct PanningControlLayout : GUIKIT::FramedVerticalLayout {
    
    struct TopLayout : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox active;
        GUIKIT::Label leftChannel;
        SliderLayout leftMix;
        SliderLayout rightMix;
        
        TopLayout();
    } top;
    
    struct BottomLayout : GUIKIT::HorizontalLayout {
        GUIKIT::Label rightChannel;
        SliderLayout leftMix;
        SliderLayout rightMix;
        
        BottomLayout();
    } bottom;
    
    PanningControlLayout();
};

struct AudioLayout : GUIKIT::VerticalLayout {

    AudioControlLayout control;
    SliderLayout latency;    
    SliderLayout volume;                
    
	GUIKIT::FramedVerticalLayout frame;
    BassControlLayout bass;
    ReverbControlLayout reverb;
    PanningControlLayout panning;        

    auto translate() -> void;
    auto updateLatencySlider() -> void;
    auto build100PercentSetting(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void;

    AudioLayout();
};
