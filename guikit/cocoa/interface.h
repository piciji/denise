
#import <Cocoa/Cocoa.h>

@interface CocoaDelegate : NSObject <NSApplicationDelegate> {}
@end

@interface CocoaWindow : NSWindow <NSWindowDelegate> {
@public
    GUIKIT::Window* window;
    NSMenu* menuBar;
    NSMenu* menuBarContext;
    NSTextField* statusBar;
}
@end

@interface CocoaTimer : NSObject {
@public
    GUIKIT::Timer* timer;
    NSTimer* instance;
}
@end

@interface CocoaMenu : NSMenuItem <NSMenuDelegate> {
@public
    NSMenu* cocoaMenu;
}
@end

@interface CocoaMenuItem : NSMenuItem {
@public
    GUIKIT::MenuItem* menuItem;
}
@end

@interface CocoaMenuCheckItem : NSMenuItem {
@public
    GUIKIT::MenuCheckItem* menuCheckItem;
}
@end

@interface CocoaMenuRadioItem : NSMenuItem {
@public
    GUIKIT::MenuRadioItem* menuRadioItem;
}
@end

@interface CocoaLabel : NSTextField {
@public
    GUIKIT::Label* label;
}
@end

@interface CocoaLineEdit : NSTextField <NSTextFieldDelegate> {
@public
    GUIKIT::LineEdit* lineEdit;
}
@end

@interface CocoaButton : NSButton {
@public
    GUIKIT::Button* button;
}
@end

@interface CocoaCheckButton : NSButton {
@public
    GUIKIT::CheckButton* checkButton;
}
@end

@interface CocoaCheckBox : NSButton {
@public
    GUIKIT::CheckBox* checkBox;
}
@end

@interface CocoaComboButton : NSPopUpButton {
@public
    GUIKIT::ComboButton* comboButton;
}
@end

@interface CocoaVerticalSlider : NSSlider {
@public
    GUIKIT::Slider* slider;
}
@end

@interface CocoaHorizontalSlider : NSSlider {
@public
    GUIKIT::Slider* slider;
}
@end

@interface CocoaRadioBox : NSButton {
@public
    GUIKIT::RadioBox* radioBox;
}
@end

@interface CocoaProgressBar : NSProgressIndicator {
@public
    GUIKIT::ProgressBar* progressBar;
}
@end

@interface CocoaViewport : NSView {
@public
    GUIKIT::Viewport* viewport;
    NSTrackingArea* trackingArea;
}
@end

@interface CocoaListViewContent : NSTableView {}
@end

@interface CocoaListViewCell : NSTextFieldCell {
@public
    GUIKIT::ListView* listView;
}
@end

@interface CocoaListView : NSScrollView <NSTableViewDelegate, NSTableViewDataSource> {
@public
    GUIKIT::ListView* listView;
    CocoaListViewContent* content;
    NSFont* font;
}
@end

@interface CocoaFrame : NSBox {}
@end

@interface CocoaTabFrame : NSTabView <NSTabViewDelegate> {
@public
    GUIKIT::pTabFrame* p;
}
@end

@interface CocoaTabFrameItem : NSTabViewItem {
@public
    GUIKIT::pTabFrame* p;
    CocoaTabFrame* cocoaTabFrame;
}
@end

@interface CocoaTreeViewContent : NSOutlineView {}
@end

@interface CocoaTreeViewCell : NSTextFieldCell {
@public
    GUIKIT::TreeView* treeView;
}
@end

@interface CocoaTreeView : NSScrollView <NSOutlineViewDelegate, NSOutlineViewDataSource> {
@public
    GUIKIT::TreeView* treeView;
    NSOutlineView* content;
    NSFont* font;
}
@end

@interface TreeViewWrapper : NSObject {
@public
    GUIKIT::TreeViewItem* treeViewItem;
}
@end

@interface CocoaSquareCanvas : NSImageView {
@public
    GUIKIT::SquareCanvas* squareCanvas;
}
@end
        
@interface CocoaHyperlink : NSTextField {
@public
    GUIKIT::Hyperlink* hyperlink;
}
@end

@interface CocoaFileDialog : NSObject<NSOpenSavePanelDelegate> {
@public
    GUIKIT::BrowserWindow* browserWindow;
}
@end
