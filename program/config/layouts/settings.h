
struct LangLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::ListView listView;
    LangLayout();
};

struct SwitchesLayout : GUIKIT::FramedVerticalLayout {
	GUIKIT::CheckBox pause;
    GUIKIT::CheckBox autostartDragnDrop;
    GUIKIT::CheckBox saveSettingsOnExit;
    GUIKIT::CheckBox openFullscreen;
    GUIKIT::CheckBox alternateSoftwarePreview;
    GUIKIT::CheckBox questionMediaWrite;
    GUIKIT::CheckBox threadedEmu;
    SwitchesLayout();
};

struct EmuSelectionLayout : GUIKIT::FramedHorizontalLayout {
    struct Core {
        GUIKIT::CheckBox* checkBox;
        Emulator::Interface* emulator;
    };

    std::vector<Core> cores;

    EmuSelectionLayout();
};

struct PreviewLayout : GUIKIT::FramedVerticalLayout {
    
    struct Top : GUIKIT::HorizontalLayout {
        GUIKIT::Label fontSize;
        GUIKIT::ComboButton fontSizeCombo;
        GUIKIT::Label dialogFontSize;
        GUIKIT::ComboButton dialogFontSizeCombo;

        struct Option : GUIKIT::VerticalLayout {
            GUIKIT::CheckBox tooltips;
            GUIKIT::CheckBox commodoreHighlight;

            Option();
        } option;

        Top();
    } top;
    
    struct Bottom : GUIKIT::HorizontalLayout {
        GUIKIT::Label dialog;
        SliderLayout dialogWidth;
        SliderLayout dialogHeight;
        
        Bottom();
    } bottom;     
        
    GUIKIT::ListView previewBox;
    
    PreviewLayout();
};

struct AboutLayout : GUIKIT::FramedHorizontalLayout {
    
    struct Left : GUIKIT::VerticalLayout {
        GUIKIT::Label author;
        GUIKIT::Label license;
        GUIKIT::Label version;        
    } left;

    GUIKIT::ImageView denise;
    
    struct Right : GUIKIT::VerticalLayout {
        GUIKIT::Hyperlink icons8;
        GUIKIT::Hyperlink trackersWorld;
    } right;

    AboutLayout();
};

struct SettingsLayout : GUIKIT::VerticalLayout {

    GUIKIT::HorizontalLayout upperLayout;
    LangLayout lang;
    SwitchesLayout switches;    
    AboutLayout about;    
    PreviewLayout previewLayout;
    EmuSelectionLayout emuSelection;
    std::vector<GUIKIT::Image*> images;
    GUIKIT::Timer previewTimer;
    std::vector<std::string> langIdents;
    GUIKIT::Image denise;

    auto setLang() -> void;
    auto changeLang() -> void;
    auto addLangImage(unsigned selection, std::string file) -> void;
    auto setPreviewContent() -> void;
    auto translate() -> void;
    auto removePreview() -> void;
    SettingsLayout();
    ~SettingsLayout();
};
