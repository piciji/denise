
struct VideoBaseLayout : GUIKIT::VerticalLayout {

    struct View : GUIKIT::FramedVerticalLayout {
        struct Mode : GUIKIT::HorizontalLayout {
            GUIKIT::RadioBox palette;
            GUIKIT::RadioBox spectrum;
            GUIKIT::RadioBox rgb;
            GUIKIT::RadioBox svideoCpu;
            GUIKIT::RadioBox svideoGpu;
            GUIKIT::RadioBox externGpu;

            GUIKIT::Widget spacer;
            GUIKIT::Button reset;

            Mode(bool withSpectrum);
        } mode;

        struct Option : GUIKIT::HorizontalLayout {
            GUIKIT::CheckBox newLuma;
            GUIKIT::CheckBox tvGamma;
            GUIKIT::CheckBox linearInterpolation;

            Option(bool withSpectrum);
        } option;

        SliderLayout saturation;
        SliderLayout gamma;
        SliderLayout brightness;
        SliderLayout contrast;
        SliderLayout phase;
        SliderLayout interlace;
        SliderLayout scanlines;

        View(bool withSpectrum);
    } view;

    struct Encoding : GUIKIT::FramedVerticalLayout {
        SliderLayout phaseError;
        SliderLayout hanoverBars;
        SliderLayout blur;

        Encoding();
    } encoding;

    struct LumaDelay : GUIKIT::FramedVerticalLayout {
        SliderLayout lumaRise;
        SliderLayout lumaFall;

        LumaDelay();
    } lumaDelay;

    VideoBaseLayout(bool withSpectrum);
};

struct VideoInternLayout : GUIKIT::VerticalLayout {

    struct Misc : GUIKIT::FramedVerticalLayout {
        struct Option : GUIKIT::HorizontalLayout {
            GUIKIT::CheckBox hires;
            GUIKIT::CheckBox distortionHires;

            Option();
        } option;

        SliderLayout lightFromCenter;
        SliderLayout luminance;

        Misc();
    } misc;

    struct Subsampling : GUIKIT::FramedVerticalLayout {
        SliderLayout firFilter;

        struct FirSharp : GUIKIT::HorizontalLayout {
            GUIKIT::RadioBox sharpLeft;
            GUIKIT::RadioBox natural;
            GUIKIT::RadioBox sharpRight;

            FirSharp();
        } firSharp;

        Subsampling();
    } subsampling;

    struct Mask : GUIKIT::FramedVerticalLayout {
        SliderLayout level;
        SliderLayout luminance;

        struct Type : GUIKIT::HorizontalLayout {
            GUIKIT::Label label;
            GUIKIT::RadioBox apertureMask;
            GUIKIT::RadioBox shadowMask;
            GUIKIT::RadioBox slotMask;

            Type();
        } type;

        SliderLayout dpi;
        SliderLayout pitch;

        Mask();
    } mask;

    struct Bloom : GUIKIT::FramedVerticalLayout {
        SliderLayout glow;
        SliderLayout radius;
        SliderLayout variance;
        SliderLayout weight;

        Bloom();
    } bloom;

    VideoInternLayout();
};

struct VideoGlitchLayout : GUIKIT::VerticalLayout {

    struct Crt : GUIKIT::FramedVerticalLayout {
        SliderLayout lumaNoise;
        SliderLayout chromaNoise;
        SliderLayout randomLineOffset;
        SliderLayout radialDistortion;

        Crt();
    } crt;

    struct VicII : GUIKIT::FramedVerticalLayout {
        GUIKIT::Button toggleAll;
        SliderLayout aec;
        SliderLayout ba;
        SliderLayout phi0;
        SliderLayout ras;
        SliderLayout cas;

        VicII();
    } vicII;

    VideoGlitchLayout(bool withVic);
};

struct VideoShaderLayout : GUIKIT::VerticalLayout {

    struct Control : GUIKIT::FramedHorizontalLayout {
        GUIKIT::Label loaded;
        GUIKIT::Button unload;

        GUIKIT::Widget spacer;
        GUIKIT::Button save;
        GUIKIT::Button load;

        Control();
    } control;

    struct Favourite : GUIKIT::FramedVerticalLayout {
        GUIKIT::ListView list;

        struct Control : GUIKIT::HorizontalLayout {
            GUIKIT::Button add;
            GUIKIT::Button remove;

            Control();
        } control;

        Favourite();
    } favourite;

    VideoShaderLayout();
};

struct VideoPassLayout : GUIKIT::FramedVerticalLayout {
    struct Load : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::Button button;

        Load();
    } load;

    struct Filter : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox unspec;
        GUIKIT::RadioBox linear;
        GUIKIT::RadioBox nearest;

        Filter();
    } filter;

    struct Wrap : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox border;
        GUIKIT::RadioBox edge;
        GUIKIT::RadioBox repeat;
        GUIKIT::RadioBox mirror;

        Wrap();
    } wrap;

    struct Scale : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox scaleInput;
        GUIKIT::RadioBox scaleAbsolute;
        GUIKIT::RadioBox scaleViewport;

        Scale();
    } scaleModeX, scaleModeY;

    SliderLayout scaleX;
    SliderLayout scaleY;

    struct Absolute : GUIKIT::HorizontalLayout {
        GUIKIT::Label labelX;
        GUIKIT::LineEdit lineX;
        GUIKIT::Label labelY;
        GUIKIT::LineEdit lineY;

        Absolute();
    } absolute;

    struct BufferType : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox unorm;
        GUIKIT::RadioBox srgb;
        GUIKIT::RadioBox fp;
        BufferType();
    } bufferType;

    GUIKIT::LineEdit alias;
    GUIKIT::CheckBox mipmap;
    GUIKIT::ComboButton modulo;

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::CheckButton lockToggle;
        GUIKIT::Button removePass;
        GUIKIT::Button appendPass;

        Control();
    } control;

    VideoPassLayout();
};

struct VideoLayout : GUIKIT::FramedHorizontalLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    GUIKIT::TreeView moduleTree;
    GUIKIT::SwitchLayout moduleSwitch;
    GUIKIT::TreeViewItem tviBase;
        GUIKIT::TreeViewItem tviIntern;
        GUIKIT::TreeViewItem tviGlitch;

    GUIKIT::TreeViewItem tviShader;
    GUIKIT::TreeViewItem tviParams;

    VideoBaseLayout layBase;
    VideoInternLayout layIntern;
    VideoGlitchLayout layGlitch;

    VideoShaderLayout layShader;

    GUIKIT::Image imgFolderOpen;
    GUIKIT::Image imgFolderClosed;
    GUIKIT::Image imgDocument;
    	
    auto translate() -> void;
    auto sliderIdent() -> std::string;
    auto updatePresets(bool reloadDriver = true) -> void;
    auto updateVisibillity() -> void;
    auto loadSettings(bool init = false) -> void;
    
    template<typename T> auto setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<void ( T value )> callBack, std::function<T ( unsigned position )> callTransfer = [](unsigned position) { return position; } ) -> void;
    auto vManager() -> VideoManager* { return VideoManager::getInstance(emulator); }
    
    VideoLayout(TabWindow* tabWindow);
};
