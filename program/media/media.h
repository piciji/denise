
#pragma once

struct Message;
struct FileSetting;

#include <thread>
#include <atomic>
#include "../../guikit/api.h"
#include "../program.h"
#include "../emuconfig/config.h"
#include "../config/slider.h"

#define SWAPPER_SLOTS 25

namespace MediaView {

struct MediaLayout;
struct MediaGroupLayout;

struct NavElement {
    GUIKIT::TreeViewItem* tvi;
    Emulator::Interface::MediaGroup* mediaGroup;
    GUIKIT::Layout* layout;
};

struct SwapperControlLayout : GUIKIT::HorizontalLayout {
	GUIKIT::CheckBox writeProtect;
    GUIKIT::Button insertButton;
    GUIKIT::Widget spacer;

    GUIKIT::Button ejectAllButton;
    GUIKIT::Button ejectButton;
    GUIKIT::Button openButton;
    
    SwapperControlLayout();
};

struct SwapperInfoLayout : GUIKIT::HorizontalLayout {
    GUIKIT::Label info1;
    GUIKIT::Label info2;

    SwapperInfoLayout();
};

struct SwapperLayout : GUIKIT::VerticalLayout {

    MediaLayout* mediaLayout;
    Emulator::Interface* emulator;
    
    GUIKIT::ListView listView;

    SwapperInfoLayout info;
    SwapperControlLayout controls;

    auto translate() -> void;
	auto getSetting( unsigned pos ) -> FileSetting*;
    auto preselectPath( ) -> std::string;
	auto savePath( std::string path ) -> void;
    auto loadSettings() -> void;
    auto clearSlot(unsigned pos) -> void;
    auto updateWP(bool state) -> void;
    
    SwapperLayout(MediaLayout* mediaLayout);
};

struct PathsLayout : GUIKIT::FramedVerticalLayout {

    struct Block : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::LineEdit edit;
        GUIKIT::Button empty;
        GUIKIT::Button select;
        
        Emulator::Interface::MediaGroup* mediaGroup;

        Block( Emulator::Interface::MediaGroup* mediaGroup );
    };
    std::vector<Block*> blocks;
    auto getBlock(Emulator::Interface::MediaGroup* mediaGroup) -> Block*;
    auto getBlockByName(const std::string& name) -> PathsLayout::Block*;
    
    PathsLayout();
};

struct DialogPreviewLayout : GUIKIT::FramedVerticalLayout {
    struct Mode : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::RadioBox noPreviewRadio;
        GUIKIT::RadioBox dialogPreviewRadio;
        GUIKIT::RadioBox softwarePreviewRadio;

        Mode();
    } mode;

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::Label fontSize;
        GUIKIT::ComboButton fontSizeCombo;

        struct Option : VerticalLayout {
            GUIKIT::CheckBox tooltips;
            GUIKIT::CheckBox commodoreHighlight;

            Option();
        } option;

        Control();
    } control;

    struct Dimension : VerticalLayout {
        SliderLayout dialogWidth;
        SliderLayout dialogHeight;

        Dimension();
    } dimension;

    GUIKIT::ListView previewBox;

    DialogPreviewLayout();

    auto updateWidgets(GUIKIT::Settings* settings, Emulator::Interface* emulator) -> void;
    auto updatePreviewContent(GUIKIT::Settings* settings, Emulator::Interface* emulator) -> void;
};

struct MediaGroupLayout : GUIKIT::FramedVerticalLayout {

    struct Block : GUIKIT::VerticalLayout {
        struct Header : GUIKIT::HorizontalLayout {
            GUIKIT::RadioBox inUse;
            GUIKIT::Label deviceName;
            GUIKIT::Button* list = nullptr;
            GUIKIT::CheckBox writeprotect;
            GUIKIT::Button eject;
            GUIKIT::Label fileName;

            Header(Emulator::Interface::Media* media, Emulator::Interface* emulator);
        } header;

        struct Selector : GUIKIT::HorizontalLayout {
            GUIKIT::LineEdit* edit = nullptr;
            GUIKIT::ComboButton* pathCombo = nullptr;
            GUIKIT::ComboButton combo;
            std::vector<GUIKIT::CheckBox*> jumpers;
            GUIKIT::Button add;
            GUIKIT::Button open;
            GUIKIT::Widget spacer;

            Selector(Emulator::Interface::Media* media, Emulator::Interface* emulator);
        } selector;

        Emulator::Interface::Media* media;
        std::vector<Emulator::Interface::Listing> listings;
        MediaGroupLayout* layout;
        bool dirty = true;
        Block(Emulator::Interface::Media* media, Emulator::Interface* emulator);
    };

    struct Control : GUIKIT::HorizontalLayout {
        GUIKIT::Button inject;
        GUIKIT::Widget spacer;
        GUIKIT::Button save;

        Control(Emulator::Interface::MediaGroup* group);
    } control;

    std::vector<Block*> blocks;
    Emulator::Interface::MediaGroup* mediaGroup;
    GUIKIT::VerticalLayout blockContainer;
    GUIKIT::MultilineEdit* hint = nullptr;
	GUIKIT::ListView listings; // for c64 disk and prg container formats
    Block* selectedBlock = nullptr;
    MediaLayout* mediaLayout;
    
    auto build(unsigned previewFontSize) -> void;
    auto updateVisibility( unsigned count, bool init = false ) -> void;    
    auto updateListing(MediaGroupLayout::Block* block) -> void;
    auto fillListing( std::vector<Emulator::Interface::Listing>& emuListings ) -> void;
    auto fillListing( std::vector<GUIKIT::BrowserWindow::Listing>& emuListings ) -> void;
    auto showOnlyConnectedDevices() -> bool;
    auto getBlock(Emulator::Interface::Media* media) -> Block*;
    auto applyFont(unsigned fontSize) -> void;
    auto setJumperSettings(Emulator::Interface::Media* media) -> void;
    auto loadSettings() -> void;

    MediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup, MediaLayout* mediaLayout );
};

struct CreatorWindow : GUIKIT::Window {
    Emulator::Interface::Media* media;
    MediaLayout* mediaLayout;

    struct DiskCreatorLayout : GUIKIT::FramedHorizontalLayout {
        GUIKIT::Label formatName;
        GUIKIT::ComboButton format;

        struct Options : GUIKIT::VerticalLayout {
            GUIKIT::CheckBox fastFileSystem;
            GUIKIT::CheckBox highDensity;
            GUIKIT::CheckBox bootable;

            Options();
        } options;

        GUIKIT::Label diskLabelName;
        GUIKIT::LineEdit diskLabel;
        GUIKIT::Widget spacer;
        GUIKIT::Button close;
        GUIKIT::Button create;

        DiskCreatorLayout(Emulator::Interface* emulator);
    } diskCreatorLayout;

    struct HdCreatorLayout : GUIKIT::FramedVerticalLayout {

        struct Creator : GUIKIT::HorizontalLayout {
            GUIKIT::Label formatName;
            GUIKIT::ComboButton format;
            GUIKIT::Label diskSizeLabel;
            GUIKIT::LineEdit diskSize;
            GUIKIT::Button close;
            GUIKIT::Button create;

            Creator(Emulator::Interface* emulator);
        } creator;

        struct Progress : GUIKIT::HorizontalLayout {
            GUIKIT::ProgressBar bar;
            GUIKIT::Label label;

            Progress();
        } progress;

        auto reset() -> void;

        std::atomic<bool> cancel = false;

        HdCreatorLayout(Emulator::Interface* emulator);
    } hdCreatorLayout;

    struct TapeCreatorLayout : GUIKIT::FramedHorizontalLayout {
        GUIKIT::Button close;
        GUIKIT::Widget spacer;
        GUIKIT::Button create;
        TapeCreatorLayout(Emulator::Interface* emulator);
    } tapeCreatorLayout;

    struct CartCreatorLayout : GUIKIT::FramedHorizontalLayout {
        GUIKIT::Button close;
        GUIKIT::Widget spacer;
        GUIKIT::Button create;
        CartCreatorLayout(Emulator::Interface* emulator);
    } cartCreatorLayout;

    auto open(Emulator::Interface::Media* media) -> void;
    auto build(MediaLayout* mediaLayout) -> void;
    auto translate() -> void;

    CreatorWindow(Emulator::Interface* emulator);
};

struct MediaLayout : GUIKIT::HorizontalLayout {
    
    EmuConfigView::TabWindow* tabWindow;
    Emulator::Interface* emulator;
    
    GUIKIT::Settings* settings;
	Message* message;
    GUIKIT::TreeViewItem* expansionParent = nullptr;

	GUIKIT::Image diskImage;
    GUIKIT::Image hdImage;
    GUIKIT::Image tapeImage;
    GUIKIT::Image expansionImage;
	GUIKIT::Image memoryImage;
	GUIKIT::Image addImage;
    GUIKIT::Image closeImage;
    GUIKIT::Image pathImage;
    GUIKIT::Image swapperImage;
    GUIKIT::Image imgFolderOpen;
    GUIKIT::Image imgFolderClosed;
    GUIKIT::Image imgDocument;
    GUIKIT::Image settingsImage;
    GUIKIT::Image searchImage;
    GUIKIT::Image binaryImage;
    GUIKIT::Image openImage;
    GUIKIT::Image ejectImg;
    
    std::vector<NavElement> navElements;

    CreatorWindow* creatorWindow = nullptr;

    GUIKIT::FramedVerticalLayout moduleFrame;
    GUIKIT::SwitchLayout moduleSwitch;
    GUIKIT::TreeView mediaTree;
    GUIKIT::CheckBox useTraps;
    struct FontSizeLayout : GUIKIT::HorizontalLayout {
        GUIKIT::Label label;
        GUIKIT::ComboButton combo;

        FontSizeLayout();
    } fontSizeLayout;

    GUIKIT::Button bootCart;
    GUIKIT::Button deactivateCart;
    
    PathsLayout pathsLayout;
    DialogPreviewLayout dialogPreviewLayout;
    SwapperLayout* swapperLayout = nullptr;

    auto build() -> void;
    auto setMediaView() -> void;
    auto setDiskSwapperView() -> void;
    auto updateSwitchLayout() -> void;
	
	auto translate() -> void;
    auto translate(NavElement& nav) -> void;
    auto updateMediaBlock(MediaGroupLayout::Block* block, FileSetting* fSetting) -> void;
    auto updateVisibility( Emulator::Interface::MediaGroup* mediaGroup, unsigned count ) -> void;
    auto updateOptionsVisibility() -> void;
    auto bindSelectorAction( MediaGroupLayout* layout ) -> void;
    auto preparePaths() -> void;
    auto preparePath(Emulator::Interface::MediaGroup& mediaGroup) -> void;
    auto updateListing( Emulator::Interface::Media* media ) -> void;
	auto savePath( std::string& groupName, std::string path ) -> void;
    auto showListing( MediaGroupLayout* layout ) -> bool;
    auto createImage( Emulator::Interface::Media* media ) -> GUIKIT::File*;
    auto insertCreatedImage(Emulator::Interface::Media* media) -> bool;
    auto showMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> void;
    auto getMediaGroupLayout( Emulator::Interface::MediaGroup* mediaGroup ) -> MediaGroupLayout*;   
    auto insertImage( MediaGroupLayout::Block* block, GUIKIT::File* file, GUIKIT::File::Item* item, int options = 0 ) -> void;
    auto insertImage( Emulator::Interface::Media* media, GUIKIT::File* file, GUIKIT::File::Item* item, int options = 0 ) -> void;
    auto ejectImage( Emulator::Interface::Media* media ) -> void;
    auto ejectImage( MediaGroupLayout::Block* block ) -> void;
    auto drop( std::string filePath, MediaGroupLayout::Block* block = nullptr ) -> void;
    auto colorListing( unsigned foregroundColor, unsigned backgroundColor ) -> void;
    auto selectionColorListing( ) -> void;
    auto fillListing(Emulator::Interface::Media* media, std::vector<GUIKIT::BrowserWindow::Listing>& listings, bool markPreview) -> void;
    auto getMediaGroupTransIdent( Emulator::Interface::MediaGroup* mediaGroup ) -> std::string;
    auto updateJumper(Emulator::Interface::Media* media) -> void;
    auto updateWriteProtection( Emulator::Interface::Media* media, bool state ) -> void;
    auto getActiveMediaGroupLayout() -> MediaGroupLayout*;
    auto resetPreview(bool light = false) -> void;
    auto convertListing( std::vector<Emulator::Interface::Listing>& emuListings, bool loadCommand ) -> std::vector<std::string>;
    auto updateListingFont( unsigned fontSize ) -> void;
    auto updateListings( ) -> void;
    auto loadSettings() -> void;
    auto getDiskSaveGroup() -> Emulator::Interface::MediaGroup*;
    auto getBlock(Emulator::Interface::Media* media) -> MediaGroupLayout::Block*;

    MediaLayout(EmuConfigView::TabWindow* tabWindow);
};

}

