
struct LangLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::ListView listView;
    LangLayout();
};

struct SwitchesLayout : GUIKIT::FramedVerticalLayout {
	GUIKIT::CheckBox pause;
    GUIKIT::CheckBox saveSettingsOnExit;
    GUIKIT::CheckBox openFullscreen;
    GUIKIT::CheckBox questionMediaWrite;
    GUIKIT::CheckBox splashScreen;
    GUIKIT::CheckBox singleInstance;
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

struct StyleLayout : GUIKIT::FramedHorizontalLayout {
    GUIKIT::RadioBox osSetting;
    GUIKIT::RadioBox dark;
    GUIKIT::RadioBox light;

    StyleLayout();
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
    GUIKIT::HorizontalLayout centerLayout;
    LangLayout lang;
    SwitchesLayout switches;    
    AboutLayout about;
    EmuSelectionLayout emuSelection;
    StyleLayout styleLayout;
    std::vector<GUIKIT::Image*> images;
    std::vector<std::string> langIdents;
    GUIKIT::Image denise;

    auto setLang() -> void;
    auto changeLang() -> void;
    auto addLangImage(unsigned selection, std::string file) -> void;
    auto translate() -> void;
    auto activateCore(Emulator::Interface* emulator) -> void;

    SettingsLayout();
    ~SettingsLayout();
};
