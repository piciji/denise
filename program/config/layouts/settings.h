
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
    SwitchesLayout();
};

struct PreviewLayout : GUIKIT::FramedVerticalLayout {
    
    struct Top : GUIKIT::HorizontalLayout {
        GUIKIT::Label fontSize;
        GUIKIT::ComboButton fontSizeCombo;
        GUIKIT::Label dialogFontSize;
        GUIKIT::ComboButton dialogFontSizeCombo;
        GUIKIT::CheckBox tooltips;
        
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
    
    struct Right : GUIKIT::VerticalLayout {
        GUIKIT::Hyperlink icons8;
    } right;

    AboutLayout();
};

struct SettingsLayout : GUIKIT::VerticalLayout {

    GUIKIT::HorizontalLayout upperLayout;
    LangLayout lang;
    SwitchesLayout switches;    
    AboutLayout about;    
    PreviewLayout previewLayout;
    std::vector<GUIKIT::Image*> images;
    GUIKIT::Timer previewTimer;
    std::vector<std::string> langIdents;

    auto setLang() -> void;
    auto changeLang() -> void;
    auto addLangImage(unsigned selection, std::string file) -> void;
    auto setPreviewContent() -> void;
    auto translate() -> void;
    auto removePreview() -> void;
    SettingsLayout();
    ~SettingsLayout();
};
