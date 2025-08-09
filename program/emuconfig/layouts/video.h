
#define PARAMS_PER_PAGE 11
#define SCALE_BOXES 5

struct VideoBaseLayout : GUIKIT::VerticalLayout {

    struct View : GUIKIT::FramedVerticalLayout {
        struct Mode : GUIKIT::HorizontalLayout {
            GUIKIT::RadioBox palette;
            GUIKIT::RadioBox spectrum;
            GUIKIT::RadioBox rgb;
            GUIKIT::RadioBox cpu;
            GUIKIT::RadioBox gpu;

            GUIKIT::Widget spacer;
            GUIKIT::CheckBox cpuFilterThreaded;
            GUIKIT::Button reset;

            Mode(bool withSpectrum);
        } mode;

        struct Option : GUIKIT::HorizontalLayout {
            GUIKIT::CheckBox newLuma;
            GUIKIT::CheckBox tvGamma;
            GUIKIT::CheckBox linearInterpolation;
            GUIKIT::Widget spacer;
            GUIKIT::Label trLabel;
            GUIKIT::RadioBox trOff;
            GUIKIT::RadioBox trOn;
            GUIKIT::RadioBox trAuto;

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
            GUIKIT::Widget spacer;
            GUIKIT::CheckBox manuell;
            GUIKIT::ImageView downloadSlang;
            GUIKIT::Label folder;
            GUIKIT::RadioBox internal;
            GUIKIT::RadioBox external;

            GUIKIT::Button prependPreset;
            GUIKIT::Button appendPreset;
            GUIKIT::Button load;

            Control();
        } control;

        struct Info : GUIKIT::HorizontalLayout {
            GUIKIT::Label label;
            GUIKIT::Label loaded;
            GUIKIT::Button imgReplacer;
            GUIKIT::CheckBox shaderCache;
            GUIKIT::Button clearCache;
            GUIKIT::Button toParams;

            Info();
        } info;

        struct Progress : GUIKIT::HorizontalLayout {
            GUIKIT::ProgressBar bar;
            GUIKIT::Label label;
            GUIKIT::Button close;

            Progress();
        } progress;

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
        GUIKIT::Widget spacer;
        GUIKIT::Button save;

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
        GUIKIT::Button save;
        GUIKIT::Widget spacer;
        GUIKIT::Button previous;
        GUIKIT::Button next;

        Control();
    } control;

    VideoParamLayout();
};

#define CONTROL_FG 0
#define CONTROL_BG 1
#define COM_BOX_FG 0
#define COM_BOX_BG 1
#define COMPONENT_R 0
#define COMPONENT_G 1
#define COMPONENT_B 2
#define COMPONENT_A 3

struct VideoScreenTextLayout : GUIKIT::VerticalLayout {

    struct ColorBoxLayout : GUIKIT::FramedVerticalLayout {

        struct Type : GUIKIT::HorizontalLayout {
            GUIKIT::Label label;
            GUIKIT::RadioBox normal;
            GUIKIT::RadioBox warning;
            GUIKIT::Widget spacer;
            GUIKIT::CheckBox onlyUrgentWarnings;
            GUIKIT::Widget spacer2;
            GUIKIT::Button reset;

            Type();
        } type;

        struct Selection : GUIKIT::HorizontalLayout {
            struct Control : GUIKIT::VerticalLayout {
                GUIKIT::SquareCanvas canvas[2];
                GUIKIT::Label hex[2];
                Control();
            } control;

            struct ComponentBox : GUIKIT::VerticalLayout {
                SliderLayout components[4];

                std::string ident;
                unsigned defaultCol;
                ComponentBox();
            } componentBox[2];

            Selection();
        } selection;

        ColorBoxLayout();
    } colorBox;

    struct Options : GUIKIT::FramedVerticalLayout {
        struct Font : GUIKIT::HorizontalLayout {
            GUIKIT::Label labelFontSize;
            GUIKIT::ComboButton fontSize;
            GUIKIT::Label labelFontType;
            GUIKIT::ComboButton fontType;
            GUIKIT::Button removeFont;
            GUIKIT::Button addFont;

            Font();
        } font;

        struct Position : GUIKIT::HorizontalLayout {
            GUIKIT::Label label;
            GUIKIT::RadioBox bottomLeft;
            GUIKIT::RadioBox bottomCenter;
            GUIKIT::RadioBox bottomRight;
            GUIKIT::RadioBox topLeft;
            GUIKIT::RadioBox topCenter;
            GUIKIT::RadioBox topRight;

            Position();
        } position;

        struct TextPadding : GUIKIT::HorizontalLayout {
            SliderLayout paddingHorizontal;
            SliderLayout paddingVertical;

            TextPadding();
        } textPadding;

        struct TextMargin : GUIKIT::HorizontalLayout {
            SliderLayout marginHorizontal;
            SliderLayout marginVertical;

            TextMargin();
        } textMargin;

        Options();
    } options;

    VideoScreenTextLayout();
};

struct VideoScreenShotLayout : GUIKIT::FramedVerticalLayout {
    struct Location : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit pathEdit;
        GUIKIT::Button standard;
        GUIKIT::Button select;

        Location();
    } location;

    struct Format : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox png;
        GUIKIT::RadioBox jpg;
        GUIKIT::RadioBox bmp;
        GUIKIT::RadioBox gif;
        GUIKIT::RadioBox tga;

        Format();
    } format;

    struct Options : GUIKIT::HorizontalLayout  {
        GUIKIT::CheckBox palete;
        Options(bool withPalete);
    } options;

    VideoScreenShotLayout(bool withPalete);
};

struct DisplayFont {
    std::string file;
    std::string name;
    unsigned index;
    uint16_t ident;

    auto getMode() -> uint8_t {
        return (ident >> 14) & 3;
    }
};

struct VideoLayout : GUIKIT::HorizontalLayout {

    TabWindow* tabWindow;
    Emulator::Interface* emulator;
    GUIKIT::FramedVerticalLayout layNav;
    GUIKIT::TreeView moduleTree;
    GUIKIT::SwitchLayout moduleSwitch;
    GUIKIT::TreeViewItem tviBase;
    GUIKIT::TreeViewItem tviScreenText;
    GUIKIT::TreeViewItem tviScreenShot;

    GUIKIT::TreeViewItem tviShader;
    std::vector<GUIKIT::TreeViewItem*> tviPasses;
    GUIKIT::TreeViewItem tviParams;

    VideoBaseLayout layBase;
    VideoShaderLayout layShader;
    VideoPassLayout layPass;
    VideoParamLayout layParam;
    VideoScreenTextLayout layScreenText;
    VideoScreenShotLayout layScreenShot;

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
    GUIKIT::Image menuImage;
    GUIKIT::Image addImage;
    GUIKIT::Image delImage;
    GUIKIT::Image gearsImage;
    GUIKIT::Image backImage;
    GUIKIT::Image screenshotImage;

    unsigned selectedPassId;
    unsigned selectedParamId;
    static std::vector<DisplayFont> displayFonts;

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
    auto buildShaderUI(ShaderPreset* preset, bool selectIt = true) -> void;
    auto buildPass(ShaderPreset* preset, ShaderPreset::Pass& pass) -> void;
    auto buildParams(TviParam& tviParam) -> void;
    auto countFloatingPoint(ShaderPreset::Param& param, int& places, int& decimalPlaces) -> void;
    auto updateMoveImg() -> void;
    auto clearErrors() -> void;
    auto showErrors(const std::vector<std::string>& errors) -> void;
    auto loadShader(std::string path) -> bool;
    auto unloadShader(bool reloadDriver = true) -> void;
    auto getShaderFolder() -> std::string;
    auto externalFolder() -> bool { return layShader.main.control.external.checked(); }
    auto openShaderFileDialog() -> std::string;
    auto presentShaderError() -> void;
    auto clearShaderError() -> void;
    auto addShaderUI() -> void;
    auto enableGPUMode(bool state) -> void;
    auto updateScreenText(bool keepFontPath) -> void;
    auto prepareColBox() -> void;
    auto fillFontTypeList() -> void;
    auto updateFontVisibilities() -> void;
    static auto addTTF(unsigned mode, const std::string& _fontFile) -> void;
    static auto getTTF(uint16_t ident) -> DisplayFont*;
    static auto getTTF(const std::string& file, int fontIndex) -> DisplayFont*;
    static auto removeTTF(const std::string& file, uint8_t mode) -> bool;
    auto updateRecordingPath() -> void;
    auto selectViewScreenshot() -> void;
    
    template<typename T> auto setSliderAction( SliderLayout* layout, std::string baseIdent, std::function<T ( unsigned position )> callTransfer = [](unsigned position) { return position; } ) -> void;
    auto vManager() -> VideoManager* { return VideoManager::getInstance(emulator); }
    
    VideoLayout(TabWindow* tabWindow);
};
