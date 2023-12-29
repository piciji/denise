
#define PARAMS_PER_PAGE 10

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

    struct Main : GUIKIT::FramedVerticalLayout {
        struct Control : GUIKIT::HorizontalLayout {
            GUIKIT::Button unload;
            GUIKIT::Button prependPreset;
            GUIKIT::Button appendPreset;

            GUIKIT::Widget spacer;
            GUIKIT::Button apply;
            GUIKIT::Button save;
            GUIKIT::Button load;

            Control();
        } control;

        struct Info : GUIKIT::HorizontalLayout {
            GUIKIT::Label label;
            GUIKIT::Label loaded;
            GUIKIT::Button toParams;

            Info();
        } info;

        Main();

        std::vector<GUIKIT::Label*> brokenLabels;
    } main;

    struct Favourite : GUIKIT::FramedVerticalLayout {
        GUIKIT::ListView list;

        struct Control : GUIKIT::HorizontalLayout {
            GUIKIT::Widget spacer;
            GUIKIT::Button remove;
            GUIKIT::Button add;

            Control();
        } control;

        Favourite();
    } favourite;

    VideoShaderLayout();
};

struct VideoPassLayout : GUIKIT::FramedVerticalLayout {

    struct Settings : GUIKIT::HorizontalLayout {
        struct Identifier : GUIKIT::VerticalLayout {
            GUIKIT::Label fileIdent;
            GUIKIT::Label filter;
            GUIKIT::Label wrap;
            GUIKIT::Label bufferType;
            GUIKIT::Label mipmap;
            GUIKIT::Label modulo;
            GUIKIT::Label scaleX;
            GUIKIT::Label scaleY;

            Identifier();
        } identifier;

        struct Data : GUIKIT::VerticalLayout {
            GUIKIT::Label fileIdent;

            struct Filter : GUIKIT::HorizontalLayout {
                GUIKIT::RadioBox unspec;
                GUIKIT::RadioBox linear;
                GUIKIT::RadioBox nearest;

                Filter();
            } filter;

            GUIKIT::Label wrap;
            GUIKIT::Label type;
            GUIKIT::Label mipmap;
            GUIKIT::Label modulo;
            GUIKIT::Label scaleX;
            GUIKIT::Label scaleY;

            Data();
        } data;

        Settings();
    } settings;

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::ImageView up;
        GUIKIT::ImageView down;
        GUIKIT::Button hide;

        Control();
    } control;

    GUIKIT::MultilineEdit info;

    VideoPassLayout();
};

struct VideoParamLayout : GUIKIT::FramedVerticalLayout {
    GUIKIT::VerticalLayout params;

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::Widget spacer;
        GUIKIT::Button previous;
        GUIKIT::Button next;

        Control();
    } control;

    VideoParamLayout();
};

struct VideoLayout : GUIKIT::HorizontalLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    GUIKIT::FramedVerticalLayout layNav;
    GUIKIT::TreeView moduleTree;
    GUIKIT::SwitchLayout moduleSwitch;
    GUIKIT::TreeViewItem tviBase;
        GUIKIT::TreeViewItem tviIntern;
        GUIKIT::TreeViewItem tviGlitch;

    GUIKIT::TreeViewItem tviShader;
    std::vector<GUIKIT::TreeViewItem*> tviPasses;
    GUIKIT::TreeViewItem tviParams;

    VideoBaseLayout layBase;
    VideoInternLayout layIntern;
    VideoGlitchLayout layGlitch;

    VideoShaderLayout layShader;
    VideoPassLayout layPass;
    VideoParamLayout layParam;

    GUIKIT::Image imgFolderOpen;
    GUIKIT::Image imgFolderClosed;
    GUIKIT::Image imgDocument;
    GUIKIT::Image pageUp;
    GUIKIT::Image pageDown;
    GUIKIT::Image pageUpGray;
    GUIKIT::Image pageDownGray;

    unsigned selectedPassId;
    unsigned selectedParamId;

    struct TviParam {
        GUIKIT::TreeViewItem* tvi;
        unsigned starts;
        unsigned counts;
    };
    std::vector<TviParam> params;

    SliderLayoutAlt* paramSliders[PARAMS_PER_PAGE];
    	
    auto translate() -> void;
    auto sliderIdent() -> std::string;
    auto updatePresets(bool reloadDriver = true) -> void;
    auto updateVisibillity() -> void;
    auto loadSettings(bool init = false) -> void;
    auto buildShaderUI(ShaderPreset* preset, bool expand = true) -> void;
    auto buildPass(ShaderPreset* preset, ShaderPreset::Pass& pass) -> void;
    auto buildParams(TviParam& tviParam) -> void;
    auto countFloatingPoint(ShaderPreset::Param& param, int& places, int& decimalPlaces) -> void;
    auto updateMoveImg() -> void;
    auto clearBrokenPaths() -> void;
    auto showBrokenPaths(std::vector<std::string>& brokenPaths) -> void;
    auto loadShader(std::string path) -> bool;
    auto unloadShader() -> void;
    
    template<typename T> auto setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<T ( unsigned position )> callTransfer = [](unsigned position) { return position; } ) -> void;
    auto vManager() -> VideoManager* { return VideoManager::getInstance(emulator); }
    
    VideoLayout(TabWindow* tabWindow);
};
