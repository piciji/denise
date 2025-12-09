
@implementation CocoaFileDialog

-(id) initWith:(GUIKIT::BrowserWindow&)browserWindowReference {
    if(self = [super init]) {
        browserWindow = &browserWindowReference;
    }
    return self;
}

- (void)panelSelectionDidChange:(id)sender {
    NSArray* curFiles = nullptr;
    NSString* curPath = nullptr;
    
    auto& state = browserWindow->state;
    
    auto& pBW = browserWindow->p;
    
    if (pBW.save) {
        curPath = (NSString*)[sender URL];
    } else
        curFiles = [sender URLs];
    
    if (pBW.multi && !pBW.save) {
        std::vector<std::string> curSelectedFiles;
        for(unsigned i = 0; i < [curFiles count]; i++) {
            NSURL* uri = [curFiles objectAtIndex:i];
            if (uri == nil)
                continue;
            const char* name = [uri fileSystemRepresentation];
            if (name) {
                curSelectedFiles.push_back(name);
              //  delete name;
            }
        }
        
        if (state.checkButton && state.checkButton->hasOrderFilesBySelection()) {
            std::vector<std::string> resultFiles;
            
            for(auto& selectedFile : browserWindow->p.sortedFiles) { // sorted selection before
                unsigned pos = 0;
                for(auto& curSelectedFile : curSelectedFiles) { // unsorted current selection
                    if (selectedFile == curSelectedFile) {
                        resultFiles.push_back( selectedFile );
                        GUIKIT::Vector::eraseVectorPos(curSelectedFiles, pos);
                        break;
                    }
                    pos++;
                }
            }
            
            for(auto& curSelectedFile : curSelectedFiles)
                resultFiles.push_back( curSelectedFile );
                
            pBW.sortedFiles = resultFiles;
        } else
            pBW.sortedFiles = curSelectedFiles;
    }
    
    if (!pBW.save) {
        if ([curFiles count] == 0)
            return;
        
        curPath = [curFiles objectAtIndex:0];
    }

    if (!curPath || curPath == nil)
        return;
    
    const char* name = [curPath fileSystemRepresentation];
    std::string path = "";
    
    if(name) {
        path = name;
      //  delete name; don't do it or autorelease causes a crash
    }

    auto listView = browserWindow->p.listView;

    bool informInsertion = pBW.multi && (pBW.sortedFiles.size() > 1);

    if ((path != browserWindow->p.selectedPath) || informInsertion) {
        if (state.onSelectionChange) {
            if (listView)
                listView->reset();
            
            if (informInsertion) {
                auto listings = state.onSelectionChange( path, true );
                
                if (listView) {
                    listView->resetRowForegroundColor(0);
                    listView->resetRowBackgroundColor(0);
                    int i = 0;
                    auto _s = GUIKIT::pFont::size([(id)listView->p.cocoaView font], " ");
                    int maxChars = state.contentView.width / _s.width;
                    
                    for(auto& file : pBW.sortedFiles) {
                        if (i >= listings.size())
                            break;
                        
                        std::string ident = GUIKIT::String::removeExtension(file, 2);
                        ident += "  " + listings[i++].entry + ": ";
                        if (ident.size() > maxChars)
                            ident = ident.substr( ident.size() - maxChars );
                        
                        listView->append({ident});
                    }
                }
            } else {
                auto rows = state.onSelectionChange(path, false);
                
                if (listView) {
                    if (state.contentView.overrideFirstRowColor) {
                        listView->setRowForegroundColor(state.contentView.firstRowForegroundColor, 0);
                        
                        listView->setRowBackgroundColor(state.contentView.firstRowBackgroundColor, 0);
                    }

                    for(auto& row : rows) {
                        listView->append({row.entry});
                    }
                    unsigned i = 0;
                    for(auto& row : rows) {
                        listView->setRowTooltip(i++, row.tooltip);
                    }
                }
            }
        }
        
        pBW.selectedPath = path;
    }
}
@end

namespace GUIKIT {

auto pBrowserWindow::fileGeneric(bool save, bool multi) -> std::vector<std::string> {
    auto& state = browserWindow.state;
    this->multi = multi;
    this->save = save;
    std::string result = "";
    std::vector<std::string> out;

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
        
        NSString* urlTextEscaped = [urlString stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];

        NSURL* url = [NSURL URLWithString:urlTextEscaped];
        
        if (save) {
            panel = [NSSavePanel savePanel];
            if(!state.title.empty()) [panel setMessage:[NSString stringWithUTF8String:state.title.c_str()]];
            [panel setDirectoryURL:url];

        } else {
            panel = [NSOpenPanel openPanel];
            [(id)panel setCanChooseDirectories:NO];
            [(id)panel setCanChooseFiles:YES];
            if (multi) {
                sortedFiles.clear();
                [(id)panel setAllowsMultipleSelection:YES];
            }

            if(!state.title.empty()) [panel setMessage:[NSString stringWithUTF8String:state.title.c_str()]];
            [panel setDirectoryURL:url];
        }
        [(id)panel setTreatsFilePackagesAsDirectories:YES];
        
        if(filtersLength > 0)
            [panel setAllowedFileTypes:filters];
        
        if (!state.textOk.empty())
            [panel setPrompt:[NSString stringWithUTF8String:state.textOk.c_str()]];
        
        buildView(save);
        
        dialogDelegate = [[CocoaFileDialog alloc] initWith: browserWindow];
        [panel setDelegate: dialogDelegate];
        
        if (!state.modal) {
            __block BrowserWindow* _browserWindow = &browserWindow;
            [panel beginWithCompletionHandler:^(NSInteger result){
             
                if (result == NSModalResponseOK) {
                    std::vector<std::string> out;
                    if (save) {
                        NSURL* uri = [panel URL];
                        if (uri) {
                            const char* name = [uri fileSystemRepresentation];
                            //const char* name = [path UTF8String];
                            if (name) {
                                out.push_back((std::string)name);
                             //   delete name;
                            }
                         //   [uri release];
                        }
                    } else {
                        NSArray* paths = [(id)panel URLs];
             
                        if( (paths != nil) && ([paths count] > 0)) {
                            for(NSURL* uri : paths) {
                                //const char* name = [path UTF8String];
                                const char* name = [uri fileSystemRepresentation];
                                if (name) {
                                    out.push_back((std::string)name);
                                    //delete name;
                                }
                            }
                            if (state.checkButton && state.checkButton->orderFilesBySelection() && sortedFiles.size()) {
                                std::vector<std::string> temp;
             
                                for (auto& sSortedFile : sortedFiles) {
                                    for(auto& sFile : out ) {
                                        if (GUIKIT::String::findString(sFile, sSortedFile))
                                            temp.push_back( sFile );
                                    }
                                }
                                out = temp;
                            }
                        }
                      //  if (paths != nil)
                        //    [paths release]; 
             
                        if (_browserWindow->state.onOkClick)
                            _browserWindow->state.onOkClick(out, _browserWindow->p.contentViewSelection());
             
                        panel = nil;
                    }
                } else if (result == NSModalResponseCancel) {
                    if (_browserWindow->state.onCancelClick)
                        _browserWindow->state.onCancelClick();
             
                    panel = nil;
                }
            }];
            
            [filters release];

            return {""};
        }
        
        NSURL* pathUri = nil;
        if([panel runModal] == NSModalResponseOK) {
            if (multi && !save) {
                NSArray* uris = [(id)panel URLs];
                for(NSURL* uri : uris) {
                    result = [uri fileSystemRepresentation];
                    out.push_back(result);
                }
                
                if (state.checkButton && state.checkButton->orderFilesBySelection() && sortedFiles.size()) {
                    std::vector<std::string> temp;
                    
                    for (auto& sSortedFile : sortedFiles) {
                        for(auto& sFile : out ) {
                            if (GUIKIT::String::findString(sFile, sSortedFile))
                                temp.push_back( sFile );
                                }
                    }
                    out = temp;
                }
            } else
                pathUri = [panel URL];
        }
        
        if(pathUri != nil) {
            const char* name = [pathUri fileSystemRepresentation];
            if(name) {
                result = name;
              //  delete name;
            }
        }
    }
    
    if (!multi || save)
        out.push_back(result);
    
    panel = nil;

    return out;
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

auto pBrowserWindow::buildView(bool save) -> void {
    auto& state = browserWindow.state;

    if ((state.buttons.size() == 0) && !state.contentView.id && !state.checkButton)
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
        if (state.contentView.overrideFirstRowColor) {
            listView->setRowForegroundColor(state.contentView.firstRowForegroundColor, 0);
            
            listView->setRowBackgroundColor(state.contentView.firstRowBackgroundColor, 0);
        }

        listView->onActivate = [this]() {
            if (browserWindow.state.contentView.onDblClick) {
                if (browserWindow.state.contentView.onDblClick( selectedPath, contentViewSelection() ) )
                    close();
            }
        };
        
        if (!state.contentView.font.empty())
            listView->setFont( state.contentView.font );
        
        listView->setSpacing( browserWindow.spacing() );
        
        maxListWidth = state.contentView.width;
        maxListHeight = state.contentView.height;
        maxContentHeight = maxListHeight + 4;
         
        listView->setGeometry({(int)_x, (int)_y, maxListWidth, maxListHeight });
            
        [listView->p.cocoaView setFrame:NSMakeRect(_x, _y, maxListWidth, maxListHeight)];

        _x = maxListWidth;
        maxContentWidth = maxListWidth;
        
        if (!state.contentView.hint.empty()) {
          //  listView->append({""});
            listView->append({state.contentView.hint});
            //listView->setRowTooltip(0, "");
            listView->setRowTooltip(0, state.contentView.hintTooltip);
        }

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
                std::vector<std::string> out;
                if (multi && sortedFiles.size()) {
                    out = sortedFiles;
                } else
                    out.push_back(selectedPath);
                
                if ( cB->onClick( out, this->contentViewSelection() ) ) {
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
            _y -= 7;
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
    
    if (state.checkButton) {
        if (listView)
            _y -= 13;
        else if (state.buttons.size())
            _y += 4;
        
        auto gCheckBox = new CheckBox;
        gCheckBox->setText(state.checkButton->text);
        gCheckBox->setFont( Font::system() );
        gCheckBox->setChecked(state.checkButton->checked);
        auto cB = state.checkButton;
        gCheckBox->onToggle = [cB, this](bool checked) {
            cB->checked = checked;
            
            if (cB->onToggle)
                cB->onToggle( checked );
        };
        auto minimumSize = gCheckBox->minimumSize();
        [gCheckBox->p.cocoaView setFrame:NSMakeRect(_x, _y, minimumSize.width + 4, minimumSize.height + 4)];
        [accessoryView addSubview: gCheckBox->p.cocoaView];
        _x += minimumSize.width + 4;
        
        maxContentWidth += minimumSize.width + 4;
        maxContentHeight += minimumSize.height + 4;
    }
    
    [accessoryView setFrame:NSMakeRect(0, 0, maxContentWidth, maxContentHeight)];
    
    [panel setAccessoryView: accessoryView];
   
    if (!save && GUIKIT::hasMinimumVersion(10, 11)) {
        if (state.contentView.id || state.buttons.size() || state.checkButton)
            [(id)panel setAccessoryViewDisclosed:YES];
        else
            [(id)panel setAccessoryViewDisclosed:NO];
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
        [panel setTreatsFilePackagesAsDirectories:YES];
        NSString* urlString = [NSString stringWithUTF8String:state.path.c_str()];
        
        NSString* urlTextEscaped = [urlString stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];
        
        NSURL* url = [NSURL URLWithString:urlTextEscaped];
        
//        NSURL* url = [NSURL URLWithString:[urlString stringByRemovingPercentEncoding]];
        [panel setDirectoryURL:url];

        if([panel runModal] == NSModalResponseOK) {
            NSArray* names = [panel URLs];
            const char* name = [[names objectAtIndex:0] fileSystemRepresentation];
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
