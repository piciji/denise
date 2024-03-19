
#define PARAMS_PER_PAGE 13
#define SCALE_BOXES 5

struct VideoBaseLayout : GUIKIT::VerticalLayout {

    struct View : GUIKIT::FramedVerticalLayout {
        struct Mode : GUIKIT::HorizontalLayout {
            GUIKIT::RadioBox palette;
            GUIKIT::RadioBox spectrum;
            GUIKIT::RadioBox rgb;
            GUIKIT::RadioBox svideoCpu;
            GUIKIT::RadioBox svideoGpu;

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

struct VideoShaderLayout : GUIKIT::VerticalLayout {

    struct Main : GUIKIT::FramedVerticalLayout {
        struct Control : GUIKIT::HorizontalLayout {
            GUIKIT::Button unload;
            GUIKIT::Button save;

            GUIKIT::Widget spacer;
            GUIKIT::Label folder;
            GUIKIT::RadioBox internal;
            GUIKIT::RadioBox external;
            GUIKIT::ImageView downloadSlang;

            GUIKIT::Button prependPreset;
            GUIKIT::Button appendPreset;
            GUIKIT::Button load;

            Control();
        } control;

        struct Info : GUIKIT::HorizontalLayout {
            GUIKIT::Label label;
            GUIKIT::Label loaded;
            GUIKIT::Button clearCache;
            GUIKIT::Button toParams;

            Info();
        } info;

        Main();

        std::vector<GUIKIT::Label*> errorLabels;
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

    struct Settings : GUIKIT::VerticalLayout {
        struct Line : GUIKIT::HorizontalLayout {
            GUIKIT::Label ident;
            GUIKIT::Label value;

            Line();
        };

        Line file;

        struct FilterLine : GUIKIT::HorizontalLayout {
            GUIKIT::Label ident;
            GUIKIT::RadioBox unspec;
            GUIKIT::RadioBox linear;
            GUIKIT::RadioBox nearest;

            FilterLine();
        } filter;

        Line wrap;
        Line bufferType;

        struct MipmapLine : GUIKIT::HorizontalLayout {
            GUIKIT::Label ident;
            GUIKIT::CheckBox checkBox;

            MipmapLine();
        } mipmap;

        Line modulo;

        struct ScaleLine : GUIKIT::HorizontalLayout {
            GUIKIT::Label ident;
            GUIKIT::Label value;
            GUIKIT::RadioBox radios[SCALE_BOXES];

            ScaleLine();
        } scaleX, scaleY;

        Settings();
    } settings;

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::ImageView up;
        GUIKIT::ImageView down;
        GUIKIT::Button disable;

        Control();
    } control;

    struct Generated : GUIKIT::HorizontalLayout {
        GUIKIT::Label errorLabel;
        GUIKIT::Widget spacer;
        GUIKIT::Button vertex;
        GUIKIT::Button fragment;

        Generated();
    } generated;

    GUIKIT::MultilineEdit errorMessage;

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

    GUIKIT::TreeViewItem tviShader;
    std::vector<GUIKIT::TreeViewItem*> tviPasses;
    GUIKIT::TreeViewItem tviParams;

    VideoBaseLayout layBase;

    VideoShaderLayout layShader;
    VideoPassLayout layPass;
    VideoParamLayout layParam;

    GUIKIT::Window codeWindow;
    GUIKIT::VerticalLayout codeLayout;
    GUIKIT::MultilineEdit codeViewer;

    GUIKIT::Image imgFolderOpen;
    GUIKIT::Image imgFolderClosed;
    GUIKIT::Image imgDocument;
    GUIKIT::Image imgError;
    GUIKIT::Image pageUp;
    GUIKIT::Image pageDown;
    GUIKIT::Image pageUpGray;
    GUIKIT::Image pageDownGray;
    GUIKIT::Image retroarch;
    GUIKIT::Image colorImage;

    unsigned selectedPassId;
    unsigned selectedParamId;

    struct TviParam {
        GUIKIT::TreeViewItem* tvi;
        std::vector<unsigned> offsets;
    };
    std::vector<TviParam> params;

    SliderLayoutAlt* paramSliders[PARAMS_PER_PAGE];
    	
    auto translate() -> void;
    auto sliderIdent() -> std::string;
    auto updatePresets(bool reloadDriver, bool reloadPreset) -> void;
    auto updateVisibillity() -> void;
    auto loadSettings(bool init = false) -> void;
    auto buildShaderUI(ShaderPreset* preset, bool expand = true) -> void;
    auto buildPass(ShaderPreset* preset, ShaderPreset::Pass& pass) -> void;
    auto buildParams(TviParam& tviParam) -> void;
    auto countFloatingPoint(ShaderPreset::Param& param, int& places, int& decimalPlaces) -> void;
    auto updateMoveImg() -> void;
    auto clearErrors() -> void;
    auto showErrors(const std::vector<std::string>& errors) -> void;
    auto loadShader(std::string path) -> bool;
    auto unloadShader() -> void;
    auto getShaderFolder() -> std::string;
    auto externalFolder() -> bool { return layShader.main.control.external.checked(); }
    auto openShaderFileDialog() -> std::string;
    auto presentShaderError() -> void;
    auto clearShaderError() -> void;
    auto addShaderUI() -> void;
    
    template<typename T> auto setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<T ( unsigned position )> callTransfer = [](unsigned position) { return position; } ) -> void;
    auto vManager() -> VideoManager* { return VideoManager::getInstance(emulator); }
    
    VideoLayout(TabWindow* tabWindow);
};
