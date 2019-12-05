
struct VideoSliderLayout : GUIKIT::HorizontalLayout {    
    GUIKIT::Label name;
    GUIKIT::CheckBox active;
    GUIKIT::Label value;
    GUIKIT::HorizontalSlider slider;
    
    std::string unit = "";
    bool withActivator;

    auto getLabelMinimumWidth() -> unsigned;
    VideoSliderLayout(bool withActivator = false, std::string unit = "%");
};

struct VideoModeLayout : GUIKIT::HorizontalLayout {
    GUIKIT::RadioBox palette;
    GUIKIT::RadioBox spectrum; 
    GUIKIT::RadioBox crtNone;
    GUIKIT::RadioBox crtCpu;
    GUIKIT::RadioBox crtGpu;    
    GUIKIT::RadioBox pal;
    GUIKIT::RadioBox ntsc;

    GUIKIT::Widget spacer;
    GUIKIT::Button reset;
    
    VideoModeLayout(bool withSpectrum);
};

struct VideoOptionLayout : GUIKIT::HorizontalLayout {
    GUIKIT::CheckBox integerScaling;
    GUIKIT::CheckBox newLuma;     
    GUIKIT::CheckBox crtRealGamma;
    
    VideoOptionLayout(bool withSpectrum);
};

struct VideoBaseLayout : GUIKIT::FramedVerticalLayout {
    VideoModeLayout mode;
    VideoOptionLayout option;
            
    VideoSliderLayout saturation;
    VideoSliderLayout gamma;
    VideoSliderLayout brightness;        	
    VideoSliderLayout contrast;   
    VideoSliderLayout phase;  
    
    VideoBaseLayout(bool withSpectrum);
};

struct VideoCrtLayout : GUIKIT::FramedVerticalLayout {
    VideoSliderLayout phaseError; 
    VideoSliderLayout hanoverBars;
    VideoSliderLayout scanlines;
    VideoSliderLayout blur;   
    VideoSliderLayout lumaRise;
    VideoSliderLayout lumaFall;
    
    VideoCrtLayout();
};

struct VideoFirSharpLayout : GUIKIT::HorizontalLayout {   
    GUIKIT::RadioBox sharpLeft;
    GUIKIT::RadioBox natural;
    GUIKIT::RadioBox sharpRight;
    
    VideoFirSharpLayout();
};

struct VideoGpuOptionLayout : GUIKIT::HorizontalLayout {
    GUIKIT::CheckBox hires;
    GUIKIT::CheckBox distortionHires;
    
    VideoGpuOptionLayout();
};

struct VideoMaskTypeLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label type;
    GUIKIT::RadioBox apertureMask;
    GUIKIT::RadioBox shadowMask;
    GUIKIT::RadioBox slotMask;
    
    auto getLabelMinimumWidth() -> unsigned;
    VideoMaskTypeLayout();
};

struct VideoGpuBaseLayout : GUIKIT::FramedVerticalLayout {
    
    VideoGpuOptionLayout option;
    VideoSliderLayout firFilter;
    VideoFirSharpLayout firSharp;
    VideoSliderLayout lightFromCenter;
    VideoSliderLayout luminance;
    
    VideoGpuBaseLayout();
};

struct VideoMaskLayout : GUIKIT::FramedVerticalLayout {
    
    VideoSliderLayout level;
    VideoSliderLayout luminance;
    VideoMaskTypeLayout type;
    VideoSliderLayout dpi;
    VideoSliderLayout pitch;
    
    VideoMaskLayout();
};

struct VideoBloomLayout : GUIKIT::FramedVerticalLayout {
    VideoSliderLayout glow;
    VideoSliderLayout radius;
    VideoSliderLayout variance;
    VideoSliderLayout weight;
    
    VideoBloomLayout();
};

struct VideoCrtGlitchLayout : GUIKIT::FramedVerticalLayout {
    VideoSliderLayout lumaNoise;
    VideoSliderLayout chromaNoise;
    VideoSliderLayout randomLineOffset;
    VideoSliderLayout radialDistortion;
    
    VideoCrtGlitchLayout();
};

struct VideoVicIIGlitchLayout : GUIKIT::FramedVerticalLayout {
    GUIKIT::Button toggleAll;
    VideoSliderLayout aec;
    VideoSliderLayout ba;
    VideoSliderLayout phi0;
    VideoSliderLayout ras;
    VideoSliderLayout cas;
    
    VideoVicIIGlitchLayout();
};

struct VideoLayout : GUIKIT::TabFrameLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::VerticalLayout tab1;
        VideoBaseLayout base;
        VideoCrtLayout crt;

    GUIKIT::VerticalLayout tab2;
        VideoGpuBaseLayout gpuBase;
        VideoMaskLayout mask;
        VideoBloomLayout bloom;        
        
    GUIKIT::VerticalLayout tab3;
        VideoCrtGlitchLayout crtGlitch;
        VideoVicIIGlitchLayout vicIIGlitch;
    	
    auto translate() -> void;
    auto sliderIdent() -> std::string;
    auto updatePresets() -> void;
    auto updateVisibillity() -> void;
    
    template<typename T> auto setSliderAction( VideoSliderLayout* layout, std::string baseIdent, std::function<void ( T value )> callBack, std::function<T ( unsigned position )> callTransfer = [](unsigned position) { return position; } ) -> void;
    auto vManager() -> VideoManager* { return VideoManager::getInstance(emulator); }
    
    VideoLayout(TabWindow* tabWindow);
};
