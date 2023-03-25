
struct VideoModeLayout : GUIKIT::HorizontalLayout {
    GUIKIT::RadioBox palette;
    GUIKIT::RadioBox spectrum; 
    GUIKIT::RadioBox rgb;
    GUIKIT::RadioBox svideoCpu;
    GUIKIT::RadioBox svideoGpu;

    GUIKIT::Widget spacer;
    GUIKIT::Button reset;
    
    VideoModeLayout(bool withSpectrum);
};

struct VideoOptionLayout : GUIKIT::HorizontalLayout {
    GUIKIT::CheckBox newLuma;
    GUIKIT::CheckBox tvGamma;
	GUIKIT::CheckBox linearInterpolation;
    
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
    SliderLayout scanlines;
    SliderLayout interlace;
    
    VideoBaseLayout(bool withSpectrum);
};

struct VideoEncodingLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout phaseError; 
    SliderLayout hanoverBars;
    SliderLayout blur;

    VideoEncodingLayout();
};

struct VideoLumaDelayLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout lumaRise;
    SliderLayout lumaFall;

    VideoLumaDelayLayout();
};

struct VideoMaskTypeLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label type;
    GUIKIT::RadioBox apertureMask;
    GUIKIT::RadioBox shadowMask;
    GUIKIT::RadioBox slotMask;
    
    VideoMaskTypeLayout();
};

struct VideoGpuMiscLayout : GUIKIT::FramedVerticalLayout {

    struct Options : GUIKIT::HorizontalLayout {
        GUIKIT::CheckBox hires;
        GUIKIT::CheckBox distortionHires;

        Options();
    } options;

    SliderLayout lightFromCenter;
    SliderLayout luminance;

    VideoGpuMiscLayout();
};

struct VideoSubsamplingLayout : GUIKIT::FramedVerticalLayout {
    SliderLayout firFilter;

    struct FirSharpLayout : GUIKIT::HorizontalLayout {
        GUIKIT::RadioBox sharpLeft;
        GUIKIT::RadioBox natural;
        GUIKIT::RadioBox sharpRight;

        FirSharpLayout();
    } firSharp;

    VideoSubsamplingLayout();
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
        VideoEncodingLayout encoding;
        VideoLumaDelayLayout lumaDelay;

    GUIKIT::VerticalLayout tab2;
        VideoGpuMiscLayout gpuMisc;
        VideoSubsamplingLayout subsampling;
        VideoMaskLayout mask;
        VideoBloomLayout bloom;        
        
    GUIKIT::VerticalLayout tab3;
        VideoCrtGlitchLayout crtGlitch;
        VideoVicIIGlitchLayout vicIIGlitch;
    	
    auto translate() -> void;
    auto sliderIdent() -> std::string;
    auto updatePresets(bool reloadDriver = true) -> void;
    auto updateVisibillity() -> void;
    auto loadSettings(bool init = false) -> void;
    
    template<typename T> auto setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<void ( T value )> callBack, std::function<T ( unsigned position )> callTransfer = [](unsigned position) { return position; } ) -> void;
    auto vManager() -> VideoManager* { return VideoManager::getInstance(emulator); }
    
    VideoLayout(TabWindow* tabWindow);
};
