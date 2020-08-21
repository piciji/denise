
struct AudioControlLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label frequencyLabel;
    GUIKIT::ComboButton frequencyCombo;
    GUIKIT::Label maxRateLabel;
    GUIKIT::LineEdit maxRateEdit;
    
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


struct AudioLayout : GUIKIT::VerticalLayout {

    AudioControlLayout control;
    SliderLayout latency;    
    SliderLayout volume;                
    
	GUIKIT::FramedVerticalLayout frame;
    BassControlLayout bass;
    ReverbControlLayout reverb;
    
    DriverLayout driverLayout;

    auto translate() -> void;
    auto updateLatencySlider() -> void;
    auto buildReverbSetting(SliderLayout* sliderLayout, std::string ident, float defaultVal) -> void;

    AudioLayout();
};
