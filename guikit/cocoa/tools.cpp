
@implementation CocoaTimer : NSObject

-(id) initWith:(GUIKIT::Timer&)timerReference {
    if(self = [super init]) {
        timer = &timerReference;
        instance = nil;
    }
    return self;
}

-(NSTimer*) instance {
    return instance;
}

-(void) update {
    if(instance) {
        [instance invalidate];
        instance = nil;
    }
    if(!timer->enabled()) return;

    instance = [NSTimer
        scheduledTimerWithTimeInterval:timer->interval() / 1000.0
        target:self selector:@selector(run:) userInfo:nil repeats:YES];
}

-(void) run:(NSTimer*)instance {
    if(timer->onFinished) timer->onFinished();
}
@end

namespace GUIKIT {

auto NSMakeImage(Image& image, unsigned width, unsigned height) -> NSImage* {
    if(image.empty()) return nil;

    NSImage* cocoaImage = [[NSImage alloc] initWithSize:NSMakeSize(image.width, image.height)];
    NSBitmapImageRep* bitmap = [[[NSBitmapImageRep alloc]
                            initWithBitmapDataPlanes:nil
                            pixelsWide:image.width pixelsHigh:image.height
                            bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES
                            isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace
                            bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                            bytesPerRow:image.width * 4 bitsPerPixel:32
                            ] autorelease];
    memcpy([bitmap bitmapData], image.data, image.height * image.width * 4);
    [cocoaImage addRepresentation:bitmap];

    if (![cocoaImage isValid]) return nil;

    if(width && height) {
        [cocoaImage setScalesWhenResized:YES];
        NSSize newSize = NSMakeSize(width, height);
        NSImage* resizedImage = [[[NSImage alloc] initWithSize: newSize] autorelease];
        [resizedImage lockFocus];
        [cocoaImage setSize: newSize];
        [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationHigh];
        [cocoaImage compositeToPoint:NSZeroPoint operation:NSCompositeCopy];
        [resizedImage unlockFocus];
        return resizedImage;
    }
    return cocoaImage;
}

//timer
pTimer::pTimer(Timer& timer) : timer(timer) {
    @autoreleasepool {
        cocoaTimer = [[CocoaTimer alloc] initWith:timer];
    }
}

pTimer::~pTimer() {
    @autoreleasepool {
        if([cocoaTimer instance]) [[cocoaTimer instance] invalidate];
        [cocoaTimer release];
    }
}

auto pTimer::setEnabled(bool enabled) -> void {
    @autoreleasepool {
        [cocoaTimer update];
    }
}

auto pTimer::setInterval(unsigned interval) -> void {
    @autoreleasepool {
        [cocoaTimer update];
    }
}
//system
auto pSystem::getUserDataFolder() -> std::string {
    std::string out = "";
    struct passwd* userinfo = getpwuid(getuid());
    out = userinfo->pw_dir;
    out = File::beautifyPath(out);
    if (out.length() > 0) out += "Library/Application Support/";
    return out;
}

auto pSystem::getResourceFolder(std::string appIdent) -> std::string {
    std::string out = "";
    char buffer[PATH_MAX];
    CFBundleRef bundle = CFBundleGetMainBundle();

    if (bundle) {
        CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(bundle);

        if (resourceURL) {
            if(CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8*)buffer, PATH_MAX)) {
                out = (std::string)buffer;
            }
            CFRelease(resourceURL);
        }
    }
    return out;
}
    
auto pSystem::getWorkingDirectory() -> std::string {
    NSFileManager* filemgr;
    NSString* currentpath;
    
    filemgr = [[NSFileManager alloc] init];
    
    currentpath = [filemgr currentDirectoryPath];
    
    return [currentpath UTF8String];
}

auto pSystem::getDesktopSize() -> Size {
    @autoreleasepool {
        NSRect primary = [[[NSScreen screens] objectAtIndex:0] frame];
        return {unsigned(primary.size.width), unsigned(primary.size.height)};
    }
}

auto pSystem::sleep(unsigned milliSeconds) -> void {
    usleep( milliSeconds * 1000 );
}
//drag'n'drop
auto DropPathsOperation(id<NSDraggingInfo> sender) -> NSDragOperation {
    NSPasteboard* pboard = [sender draggingPasteboard];
    if([[pboard types] containsObject:NSFilenamesPboardType]) {
        if([sender draggingSourceOperationMask] & NSDragOperationGeneric) return NSDragOperationGeneric;
    }
    return NSDragOperationNone;
}

auto getDropPaths(id<NSDraggingInfo> sender) -> std::vector<std::string> {
    std::vector<std::string> paths;
    NSPasteboard* pboard = [sender draggingPasteboard];
    if([[pboard types] containsObject:NSFilenamesPboardType]) {
        NSArray* files = [pboard propertyListForType:NSFilenamesPboardType];
        for(unsigned n = 0; n < [files count]; n++) {
            std::string path = [[files objectAtIndex:n] UTF8String];
            if (File::isDir(path) && path.back() != '/') path.push_back('/');
            paths.push_back(path);
        }
    }
    return paths;
}

auto pSystem::getOSLang() -> System::Language {
    
    NSString* lang = [[NSLocale preferredLanguages] objectAtIndex:0];
    std::string str = [lang UTF8String];

    GUIKIT::String::toLowerCase( str );
        
    if (str.find("de") != std::string::npos)
        return System::Language::DE;
        
    if (str.find("fr") != std::string::npos)
        return System::Language::FR;
        
    if (str.find("us") != std::string::npos)
        return System::Language::US;
        
    return System::Language::UK;
}

    
//font
auto pFont::system(unsigned size, std::string style) -> std::string {
    if(style == "") style = "Normal";
    @autoreleasepool {
        NSFont* font = [NSFont systemFontOfSize: [NSFont systemFontSize]];
        std::string family([[font familyName] UTF8String]);

        if(size == 0) {
            CGFloat defaultFontSize = [NSFont systemFontSize];
            size = defaultFontSize / 1.5;
        }
        return family + ", " + std::to_string(size) + ", " + style;
    }
}

auto pFont::cocoaFont(std::string desc) -> NSFont* {
    std::vector<std::string> tokens = String::split(desc, ',');

    NSString* family = @"Lucida Grande";
    CGFloat size = 8.0;
    NSFontTraitMask traits = 0;

    if(tokens.at(0) != "") family = [NSString stringWithUTF8String:tokens.at(0).c_str()];
    if(tokens.size() >= 2  && String::isNumber(tokens.at(1))) size = std::stoi(tokens.at(1));
    if(tokens.size() >= 3) {
        for(unsigned i = 2; i < tokens.size(); i++) {
            std::string style = String::toLowerCase( tokens.at( i ) );
            if (String::foundSubStr(style, "bold")) traits |= NSBoldFontMask;
            if (String::foundSubStr(style, "italic")) traits |= NSItalicFontMask;
            if (String::foundSubStr(style, "narrow")) traits |= NSNarrowFontMask;
            if (String::foundSubStr(style, "expanded")) traits |= NSExpandedFontMask;
            if (String::foundSubStr(style, "condensed")) traits |= NSCondensedFontMask;
            if (String::foundSubStr(style, "smallcaps")) traits |= NSSmallCapsFontMask;
        }
    }
    size *= 1.5;  //scale to point sizes
    return [[NSFontManager sharedFontManager] fontWithFamily:family traits:traits weight:5 size:size];
}

auto pFont::size(NSFont* font, std::string text) -> Size {
    @autoreleasepool {
        NSString* cocoaText = [NSString stringWithUTF8String:text.c_str()];
        NSDictionary* fontAttributes = [NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, nil];
        NSSize size = [cocoaText sizeWithAttributes:fontAttributes];
        return {(unsigned)size.width, (unsigned)size.height};
    }
}

auto pFont::size(std::string font, std::string text) -> Size {
    @autoreleasepool {
        if(NSFont* nsFont = cocoaFont(font)) return size(nsFont, text);
    }
    
    return {0, 0};
}

auto pFont::add( CustomFont* customFont ) -> bool {
    
    NSString* _str = [NSString stringWithUTF8String: customFont->filePath.c_str()];
    
    NSURL* fontURL = [NSURL fileURLWithPath: _str];
    
    CFErrorRef error = NULL;
    if (!CTFontManagerRegisterFontsForURL((__bridge CFURLRef)fontURL, kCTFontManagerScopeProcess, &error))
    {
        return false;
    }
    
    return true;
}

    
}
