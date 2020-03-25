
struct LangLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::ListView listView;
    LangLayout();
};

struct SwitchesLayout : GUIKIT::FramedVerticalLayout {
    GUIKIT::CheckBox fullscreenStatusbar;
	GUIKIT::CheckBox pause;
    GUIKIT::CheckBox autostartDragnDrop;
    GUIKIT::CheckBox saveSettingsOnExit;
    GUIKIT::CheckBox openFullscreen;
    GUIKIT::CheckBox enableSoftwarePreview;
    SwitchesLayout();
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
    std::vector<GUIKIT::Image*> images;

    auto setLang() -> void;
    auto changeLang() -> void;
    auto addLangImage(unsigned selection, std::string file) -> void;
    auto translate() -> void;
    SettingsLayout();
    ~SettingsLayout();
};
