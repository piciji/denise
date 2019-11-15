
#include "../api.h"

class ExtendedGui : public GUIKIT::Window {
    GUIKIT::FramedHorizontalLayout hmain;

        GUIKIT::FramedVerticalLayout vleft;
            GUIKIT::Label l3;

            GUIKIT::TabFrameLayout tleft;
                GUIKIT::HorizontalLayout hl1;
                    GUIKIT::ComboButton co1;
                    GUIKIT::Button b1;
            GUIKIT::Label l2;

        GUIKIT::ListView lv;

        GUIKIT::TabFrameLayout tright;
            GUIKIT::FramedHorizontalLayout hr1;
                GUIKIT::CheckBox cb1;
                GUIKIT::Widget spacer;
                GUIKIT::HorizontalSlider sl1;
            GUIKIT::FramedVerticalLayout vr1;
                GUIKIT::Label l1;
                GUIKIT::LineEdit le1;

public:
    void build();
};

class TabGui : public GUIKIT::Window {
public:
    GUIKIT::VerticalLayout vo;
        GUIKIT::HorizontalSlider ho;
        GUIKIT::TabFrameLayout tab;
            GUIKIT::FramedHorizontalLayout h1;
                GUIKIT::CheckBox cb;
                GUIKIT::LineEdit le;
                GUIKIT::HorizontalSlider hs1;
            GUIKIT::FramedVerticalLayout vx;
                GUIKIT::FramedHorizontalLayout h2;
                    GUIKIT::Label l2;
                    GUIKIT::Button bu;
                    GUIKIT::Label l5;
                    GUIKIT::ComboButton combo;
                    GUIKIT::RadioBox rb;
            GUIKIT::TabFrameLayout tab1;
                GUIKIT::VerticalLayout vl;
                    GUIKIT::LineEdit le1;
                    GUIKIT::Button bu1;
                    GUIKIT::Label la1;

                GUIKIT::FramedHorizontalLayout h3;
                    GUIKIT::Label l1;
                    GUIKIT::TabFrameLayout tab2;
                        GUIKIT::VerticalLayout v2;
                            GUIKIT::VerticalSlider vs;
                        GUIKIT::VerticalLayout v3;
                            GUIKIT::HorizontalSlider hs;
                            GUIKIT::Button b4;
                            GUIKIT::ListView lv;

public:
    void build();
    void setimg();
};

class SubGui : public GUIKIT::Window {
    ExtendedGui& egui;

    GUIKIT::FramedHorizontalLayout hl;
    GUIKIT::FramedHorizontalLayout hlx;

        GUIKIT::VerticalLayout vl1;
            GUIKIT::FramedHorizontalLayout hl1;
            GUIKIT::HorizontalLayout hl2;

        GUIKIT::VerticalLayout vl2;

    GUIKIT::LineEdit le;
    GUIKIT::Label la;
    GUIKIT::Button bu;
    GUIKIT::CheckBox cb;
    GUIKIT::ListView lv;

public:
    SubGui(ExtendedGui& egui) : egui(egui) {}
    void build();
};

class MainGui : public GUIKIT::Window {
    SubGui& sgui;
    TabGui& tgui;

    GUIKIT::Menu systemMenu;
        GUIKIT::MenuItem poweronItem;
        GUIKIT::MenuItem poweroffItem;
        GUIKIT::MenuItem disksItem;
        GUIKIT::MenuItem diskSwapperItem;
        GUIKIT::MenuItem systemItem;
        GUIKIT::MenuItem saveStateItem;

    GUIKIT::Menu inputMenu;
        GUIKIT::Menu port1Menu;
            GUIKIT::MenuRadioItem port1noneItem;
            GUIKIT::MenuRadioItem port1joyItem;
            GUIKIT::MenuRadioItem port1mouseItem;
        GUIKIT::Menu port2Menu;
            GUIKIT::MenuRadioItem port2noneItem;
            GUIKIT::MenuRadioItem port2joyItem;
            GUIKIT::MenuRadioItem port2mouseItem;
        GUIKIT::MenuItem inputItem;

    GUIKIT::Menu settingsMenu;
        GUIKIT::Menu filterMenu;
            GUIKIT::MenuRadioItem videoNearestItem;
            GUIKIT::MenuRadioItem videoLinearItem;
             GUIKIT::MenuSeparator sep;
            GUIKIT::MenuCheckItem audioLowpassItem;

        GUIKIT::Menu regionMenu;
            GUIKIT::MenuRadioItem palItem;
            GUIKIT::MenuRadioItem ntscItem;

        GUIKIT::MenuCheckItem audioSyncItem;
        GUIKIT::MenuCheckItem videoSyncItem;

        GUIKIT::MenuCheckItem muteItem;
        GUIKIT::MenuCheckItem aspectCorrectItem;
        GUIKIT::MenuCheckItem fpsItem;

        GUIKIT::MenuItem configItem;

        GUIKIT::HorizontalLayout hlayout;
        GUIKIT::FixedLayout fixed;
        GUIKIT::FixedLayout fixed2;

        GUIKIT::Label l1;
        GUIKIT::Label l2;
        GUIKIT::Label l3;
        GUIKIT::LineEdit le1;
        GUIKIT::LineEdit le2;
        GUIKIT::Button b1;
        GUIKIT::CheckButton cb1;
        GUIKIT::CheckBox cb2;
        GUIKIT::ComboButton co1;
        GUIKIT::HorizontalSlider hs;
        GUIKIT::VerticalSlider vs;

        GUIKIT::RadioBox rb1;
        GUIKIT::RadioBox rb2;
        GUIKIT::RadioBox rb3;

        GUIKIT::ListView lv1;
        GUIKIT::ListView lv2;
        GUIKIT::Viewport vp;

        GUIKIT::ProgressBar pb;

        GUIKIT::TreeView tv;
        GUIKIT::TreeViewItem tvi1;
            GUIKIT::TreeViewItem tvi11;
            GUIKIT::TreeViewItem tvi12;
        GUIKIT::TreeViewItem tvi2;
            GUIKIT::TreeViewItem tvi21;
            GUIKIT::TreeViewItem tvi22;
            GUIKIT::TreeViewItem tvi23;
                GUIKIT::TreeViewItem tvi231;
                    GUIKIT::TreeViewItem tvi2311;

        GUIKIT::TreeViewItem tvi3;

        GUIKIT::Timer stimer;
        GUIKIT::Timer stimer2;

        GUIKIT::Image img;
        GUIKIT::Image img2;
        GUIKIT::Image img3;
        GUIKIT::Image img4;
        GUIKIT::Image imgFO;
        GUIKIT::Image imgFC;
        GUIKIT::Image imgDO;

public:
    MainGui(SubGui& sgui, TabGui& tgui) : sgui(sgui), tgui(tgui) {}
    void build();
    void show();
};
