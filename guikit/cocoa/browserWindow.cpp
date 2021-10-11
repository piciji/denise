
@implementation CocoaFileDialog

-(id) initWith:(GUIKIT::BrowserWindow&)browserWindowReference {
    if(self = [super init]) {
        browserWindow = &browserWindowReference;
    }
    return self;
}

- (void)panelSelectionDidChange:(id)sender {
    NSArray* curFiles = [sender filenames];
    
    if ([curFiles count] == 0)
        return;
    
    NSString* curPath = [curFiles objectAtIndex:0];
    
    if (curPath == nil)
        return;
    
    const char* name = [curPath UTF8String];
    std::string path = "";
    auto& state = browserWindow->state;
    
    if(name)
        path = name;

    auto listView = browserWindow->p.listView;

    if (path != browserWindow->p.selectedPath) {
        if (listView)
            listView->reset();

        if (state.onSelectionChange) {
            auto rows = state.onSelectionChange(path);

            if (listView) {
                for(auto& row : rows) {
                    listView->append({row.entry});
                }
                unsigned i = 0;
                for(auto& row : rows) {
                    listView->setRowTooltip(i++, row.tooltip);
                }
            }
        }
        
        browserWindow->p.selectedPath = path;
    }
}
@end

namespace GUIKIT {

auto pBrowserWindow::file(bool save) -> std::string {
    auto& state = browserWindow.state;
    std::string result = "";

    @autoreleasepool {
        NSMutableArray* filters = [[NSMutableArray alloc] init];
        bool disableFilter = false;
        
        for(auto& filter : state.filters) {
            std::vector<std::string> tokens = String::split(filter, '(');
            if(tokens.size() != 2) continue;
            std::string part = tokens.at(1);
            part.pop_back();
            String::delSpaces(part);
            tokens = String::split(part, ',');
            for(auto& token : tokens) {
                if (token == "*") {
                    disableFilter = true;
                    break;
                }
                String::remove(token, { "*.", "*" } );
                if (!token.empty()) [filters addObject:[NSString stringWithUTF8String:token.c_str()]];
            }
            if (disableFilter) {
                filters = [[NSMutableArray alloc] init];
                break;
            }
        }
        
        NSUInteger filtersLength = [filters count];

        NSString* urlString = [NSString stringWithUTF8String:state.path.c_str()];
        NSURL* url = [NSURL URLWithString:[urlString stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
        
        if (save) {
            panel = [NSSavePanel savePanel];
            if(!state.title.empty()) [panel setMessage:[NSString stringWithUTF8String:state.title.c_str()]];
            [panel setDirectoryURL:url];

        } else {
            panel = [NSOpenPanel openPanel];
            [panel setCanChooseDirectories:NO];
            [panel setCanChooseFiles:YES];

            if(!state.title.empty()) [panel setMessage:[NSString stringWithUTF8String:state.title.c_str()]];
            [panel setDirectoryURL:url];
        }
        
        if(filtersLength > 0)
            [panel setAllowedFileTypes:filters];
        
        if (!state.textOk.empty())
            [panel setPrompt:[NSString stringWithUTF8String:state.textOk.c_str()]];
        
        buildView();
        
        dialogDelegate = [[CocoaFileDialog alloc] initWith: browserWindow];
        [panel setDelegate: dialogDelegate];
        
        if (!state.modal) {
            __block BrowserWindow* _browserWindow = &browserWindow;
            [panel beginWithCompletionHandler:^(NSInteger result){
             
                if (result == NSFileHandlingPanelOKButton) {
                    NSArray* paths = [panel filenames];
             
                    const char* name = nullptr;
                    if( (paths != nil) && ([paths count] > 0)) {
                        name = [[paths objectAtIndex:0] UTF8String];
                    }
             
                    if (name && _browserWindow->state.onOkClick)
                        _browserWindow->state.onOkClick((std::string)name, _browserWindow->p.contentViewSelection());
             
                    panel = nil;
             
                } else if (result == NSFileHandlingPanelCancelButton) {
                    if (_browserWindow->state.onCancelClick)
                        _browserWindow->state.onCancelClick();
             
                    panel = nil;
                }
            }];
            
            [filters release];

            return "";
        }
        
        NSString* path = nil;
        if([panel runModal] == NSFileHandlingPanelOKButton) {
            path = [panel filename];
        }
        
        if(path != nil) {
            const char* name = [path UTF8String];
            if(name) result = name;
        }
    }
    
    panel = nil;

    return result;
}

auto pBrowserWindow::setListings( std::vector<BrowserWindow::Listing>& listings ) -> void {
    if (listView) {
        for(auto& listing : listings) {
            listView->append({listing.entry});
        }
        unsigned i = 0;
        for(auto& listing : listings) {
            listView->setRowTooltip(i++, listing.tooltip);
        }
    }
}

auto pBrowserWindow::buildView() -> void {
    auto& state = browserWindow.state;

    if ( (state.buttons.size() == 0) && !state.contentView.id )
        return;
    
    accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];

    unsigned maxListWidth = 0;
    unsigned maxListHeight = 0;
    unsigned maxContentHeight = 0;
    unsigned maxContentWidth = 0;
    
    unsigned _x = 0;
    unsigned _y = 2;
    
    if (state.contentView.id) {
        listView = new ListView;
        listView->setHeaderText({""});
        listView->setHeaderVisible( false );
        if (state.contentView.overrideBackgroundColor)
            listView->setBackgroundColor( state.contentView.backgroundColor );
        if (state.contentView.overrideForegroundColor)
            listView->setForegroundColor( state.contentView.foregroundColor );
            
        if (state.contentView.overrideSelectionColor)
            listView->setSelectionColor( state.contentView.selectionForegroundColor, state.contentView.selectionBackgroundColor );

        listView->colorRowTooltips( state.contentView.colorTooltips );
        listView->onActivate = [this]() {
            if (browserWindow.state.contentView.onDblClick) {
                if (browserWindow.state.contentView.onDblClick( selectedPath, contentViewSelection() ) )
                    close();
            }
        };
        
        if (!state.contentView.font.empty())
            listView->setFont( state.contentView.font, state.contentView.specialFont );
        
        maxListWidth = state.contentView.width;
        maxListHeight = state.contentView.height;
        maxContentHeight = maxListHeight + 4;
         
        listView->setGeometry({(int)_x, (int)_y, maxListWidth, maxListHeight });
            
        [listView->p.cocoaView setFrame:NSMakeRect(_x, _y, maxListWidth, maxListHeight)];

        _x = maxListWidth;
        maxContentWidth = maxListWidth;
        
        [accessoryView addSubview: listView->p.cocoaView];
        
        _y = maxContentHeight - 2;
    }
    
    if (state.buttons.size()) {
        _x += 10;
        maxContentWidth += 10;
    }
    
    unsigned maxButtonWidth = 0;
    unsigned maxButtonHeight = 0;
    
    for(auto& button : state.buttons) {
        
        auto gButton = new Button;
        gButton->setText(button.text);
        gButton->setFont( Font::system() );
        auto cB = &button;
        gButton->onActivate = [cB, this]() {
            
            if (cB->onClick) {
                if ( cB->onClick( this->selectedPath, this->contentViewSelection() ) ) {
                    if (this->panel) {
                        this->close();
                    }
                }
            }
        };
        auto minimumSize = gButton->minimumSize();
        maxButtonHeight = std::max(maxButtonHeight, minimumSize.height + 4);
        
        if (listView)
            _y -= maxButtonHeight;
        
        [gButton->p.cocoaView setFrame:NSMakeRect(_x, _y, minimumSize.width + 4, minimumSize.height + 4)];
        
        if (listView) {
            _y -= 5;
            maxButtonWidth = std::max(maxButtonWidth, minimumSize.width + 4 );
        } else {
            maxButtonWidth += minimumSize.width + 4;
            maxContentHeight = maxButtonHeight + 4;
            _x += minimumSize.width + 4;
        }

        [accessoryView addSubview: gButton->p.cocoaView];
        
        buttons.push_back( gButton );
    }
    
    maxContentWidth += maxButtonWidth;
    
    [accessoryView setFrame:NSMakeRect(0, 0, maxContentWidth, maxContentHeight)];
    
    [panel setAccessoryView: accessoryView];
   
    if (GUIKIT::hasMinimumVersion(10, 11)) {
        if (state.contentView.id || state.buttons.size())
            [panel setAccessoryViewDisclosed:YES];
        else
            [panel setAccessoryViewDisclosed:NO];
    }
}
    
auto pBrowserWindow::directory() -> std::string {
    auto& state = browserWindow.state;
    std::string result;

    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        if(!state.title.empty()) [panel setMessage:[NSString stringWithUTF8String:state.title.c_str()]];
        [panel setCanChooseDirectories:YES];
        [panel setCanChooseFiles:NO];
        NSString* urlString = [NSString stringWithUTF8String:state.path.c_str()];
        NSURL* url = [NSURL URLWithString:[urlString stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
        [panel setDirectoryURL:url];

        if([panel runModal] == NSOKButton) {
            NSArray* names = [panel filenames];
            const char* name = [[names objectAtIndex:0] UTF8String];
            if(name) result = name;
        }
    }

    if(!result.empty() && (result.back() != '/')) result.push_back('/');
    return result;
}
    
auto pBrowserWindow::close() -> void {
    @autoreleasepool {
        if (visible()) {
            [panel close];
        }
    }
}
    
auto pBrowserWindow::setForeground() -> void {
    @autoreleasepool {
        if (panel)
            [panel makeKeyAndOrderFront:nil];
    }

}

auto pBrowserWindow::contentViewSelection() -> unsigned {
    return (listView && listView->selected()) ? listView->selection() : 0;
}
    
auto pBrowserWindow::detached() -> bool {
    return !browserWindow.state.modal;
}
    
auto pBrowserWindow::visible() -> bool {
    
    if (panel == nil)
        return false;
    
   // @autoreleasepool {
        return [panel isVisible] == YES;
   // }
}
    
pBrowserWindow::pBrowserWindow(BrowserWindow& browserWindow)
 : browserWindow(browserWindow) {
        
}
    
pBrowserWindow::~pBrowserWindow() {
    close();
    
    @autoreleasepool {
        if (dialogDelegate)
            [dialogDelegate release];
        
        if (listView)
            delete listView;
        
        for(auto button : buttons)
            delete button;
        
        if (accessoryView)
            [accessoryView release];
        
        if (panel)
            [panel release];
        
        panel = nil;
    }
}

}
