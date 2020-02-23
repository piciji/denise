
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
    GUIKIT::CheckBox newLuma;     
    GUIKIT::CheckBox crtRealGamma;
    
    VideoOptionLayout(bool withSpectrum);
};

struct VideoBaseLayout : GUIKIT::FramedVerticalLayout {
    VideoModeLayout mode;
    VideoOptionLayout option;
            
    SliderLayout saturation;
    SliderLayout gamma;
    SliderLayout brightness;        	
    SliderLayout contrast;   
    SliderLayout phase;  
    
    VideoBaseLayout(bool withSpectrum);
};

struct VideoCrtLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout phaseError; 
    SliderLayout hanoverBars;
    SliderLayout scanlines;
    SliderLayout blur;   
    SliderLayout lumaRise;
    SliderLayout lumaFall;
    
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
    
    VideoMaskTypeLayout();
};

struct VideoGpuBaseLayout : GUIKIT::FramedVerticalLayout {
    
    VideoGpuOptionLayout option;
    SliderLayout firFilter;
    VideoFirSharpLayout firSharp;
    SliderLayout lightFromCenter;
    SliderLayout luminance;
    
    VideoGpuBaseLayout();
};

struct VideoMaskLayout : GUIKIT::FramedVerticalLayout {
    
    SliderLayout level;
    SliderLayout luminance;
    VideoMaskTypeLayout type;
    SliderLayout dpi;
    SliderLayout pitch;
    
    VideoMaskLayout();
};

struct VideoBloomLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout glow;
    SliderLayout radius;
    SliderLayout variance;
    SliderLayout weight;
    
    VideoBloomLayout();
};

struct VideoCrtGlitchLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout lumaNoise;
    SliderLayout chromaNoise;
    SliderLayout randomLineOffset;
    SliderLayout radialDistortion;
    
    VideoCrtGlitchLayout();
};

struct VideoVicIIGlitchLayout : GUIKIT::FramedVerticalLayout {
    GUIKIT::Button toggleAll;
    SliderLayout aec;
    SliderLayout ba;
    SliderLayout phi0;
    SliderLayout ras;
    SliderLayout cas;
    
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
    
    template<typename T> auto setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<void ( T value )> callBack, std::function<T ( unsigned position )> callTransfer = [](unsigned position) { return position; } ) -> void;
    auto vManager() -> VideoManager* { return VideoManager::getInstance(emulator); }
    
    VideoLayout(TabWindow* tabWindow);
};
