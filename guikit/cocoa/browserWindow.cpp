
namespace GUIKIT {

auto pBrowserWindow::file(BrowserWindow::State& state, bool save) -> std::string {
    std::string result;

    @autoreleasepool {
        NSArray* paths = nil;
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
            NSSavePanel* panel = [NSSavePanel savePanel];
            if(!state.title.empty()) [panel setTitle:[NSString stringWithUTF8String:state.title.c_str()]];
            [panel setDirectoryURL:url];
            if(filtersLength > 0) [panel setAllowedFileTypes:filters];
            if([panel runModal] == NSOKButton) paths = [panel filenames];

        } else {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            [panel setCanChooseDirectories:NO];
            [panel setCanChooseFiles:YES];
            if(!state.title.empty()) [panel setMessage:[NSString stringWithUTF8String:state.title.c_str()]];
            [panel setDirectoryURL:url];
            if(filtersLength > 0) [panel setAllowedFileTypes:filters];
            if([panel runModal] == NSOKButton) paths = [panel filenames];
        }

        if( (paths != nil) && ([paths count] > 0)) {
            const char* name = [[paths objectAtIndex:0] UTF8String];
            if(name) result = name;
        }
        [filters release];
    }

    return result;
}

auto pBrowserWindow::directory(BrowserWindow::State& state) -> std::string {
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

}
