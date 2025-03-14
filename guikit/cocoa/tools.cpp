#include <mach/mach.h>
#include <mach/mach_time.h>

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

@implementation IntegerFormatter : NSNumberFormatter

-(id) initWith:(bool)allowNegative {
    if(self = [super init]) {
        bAllowNegative = allowNegative;
    }
    return self;
}

- (BOOL)isPartialStringValid:(NSString *)partialString
newEditingString:(NSString *__autoreleasing *)newString
errorDescription:(NSString *__autoreleasing *)error {
    for (int i = 0; i < [partialString length]; i++) {
        unichar c = [partialString characterAtIndex:i];
        if (!bAllowNegative) {
            if (!(c >= '0' && c <= '9')) return NO;
        } else {
            if (!(c >= '0' && c <= '9') && !(i == 0 && c == '-')) return NO;
        }
    }
    return YES;
}

- (NSString *)stringForObjectValue:(id)obj {
    return obj;
}

- (BOOL)getObjectValue:(__autoreleasing id *)obj
forString:(NSString *)string
errorDescription:(NSString *__autoreleasing *)error {
    *obj = string;
    return YES;
}

@end

namespace GUIKIT {

#include "versioning.cpp"
    
auto NSMakeImage(Image& image, unsigned width, unsigned height, unsigned addLines) -> NSImage* {
    if(image.empty()) return nil;

    NSImage* cocoaImage = [[NSImage alloc] initWithSize:NSMakeSize(image.width, image.height)];
    NSBitmapImageRep* bitmap = [[[NSBitmapImageRep alloc]
                            initWithBitmapDataPlanes:nil
                            pixelsWide:image.width pixelsHigh:image.height + addLines
                            bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES
                            isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace
                            bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                            bytesPerRow:image.width * 4 bitsPerPixel:32
                            ] autorelease];
    memcpy([bitmap bitmapData], image.data, image.height * image.width * 4);
    [cocoaImage addRepresentation:bitmap];

    if (![cocoaImage isValid]) return nil;

    if(width && height) {
        NSSize newSize = NSMakeSize(width, height);
        NSImage* resizedImage = [[[NSImage alloc] initWithSize: newSize] autorelease];
        [resizedImage lockFocus];
        [cocoaImage setSize: newSize];
        [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationHigh];
        [cocoaImage drawAtPoint:NSZeroPoint fromRect:NSZeroRect operation:NSCompositingOperationMultiply fraction:1.0];
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

auto pSystem::printToCmd( std::string str ) -> void {
	
	fprintf(stdout, "%s", str.c_str() );
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
auto pFont::system(unsigned size, std::string style, bool monospaced) -> std::string {
    if(style == "") style = "Normal";
    @autoreleasepool {
        NSFont* font = [NSFont systemFontOfSize: [NSFont systemFontSize]];
        std::string family([[font familyName] UTF8String]);

        if (monospaced)
            family = "Andale Mono";
        
    //    NSLog(@"%@", [[[NSFontManager sharedFontManager] availableFontFamilies] description]);
            
        if(size == 0) {
            CGFloat defaultFontSize = [NSFont systemFontSize];
            size = defaultFontSize / 1.5;
        }
        return family + ", " + std::to_string(size) + ", " + style;
    }
}

auto pFont::systemFontFile() -> std::string {
    @autoreleasepool {
        NSFont* font = [NSFont systemFontOfSize: [NSFont systemFontSize]];
        
        CTFontRef aFont = CTFontCreateWithName((CFStringRef) [font fontName], 0.0, NULL);

        CFTypeRef attribute = CTFontCopyAttribute(aFont, kCTFontURLAttribute);
        
        if(!attribute) // don't know how to access Apple System Fonts
            return "/Library/Fonts/Arial Unicode.ttf";
        
        NSURL* fontFileURL = [(NSURL*) attribute autorelease];
        
        return fontFileURL.fileSystemRepresentation;
    }
}

auto pFont::cocoaFont(const std::string& desc) -> NSFont* {
    std::vector<std::string> tokens = String::split(desc, ',');

    NSString* family = @"Lucida Grande";
    CGFloat size = 8.0;
    NSFontTraitMask traits = 0;

    if(tokens.at(0) != "") family = [NSString stringWithUTF8String:tokens[0].c_str()];
    if(tokens.size() >= 2  && String::isNumber(tokens.at(1))) size = std::stoi(tokens[1]);
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
        return {(unsigned)ceil(size.width), (unsigned)ceil(size.height)};
    }
}

auto pFont::size(std::string font, std::string text) -> Size {
    @autoreleasepool {
        if(NSFont* nsFont = cocoaFont(font)) return size(nsFont, text);
    }
    
    return {0, 0};
}

auto pFont::add( CustomFont& customFont ) -> bool {
    
    NSString* _str = [NSString stringWithUTF8String: customFont.filePath.c_str()];
    
    NSURL* fontURL = [NSURL fileURLWithPath: _str];
    
    CFErrorRef error = NULL;
    if (!CTFontManagerRegisterFontsForURL((__bridge CFURLRef)fontURL, kCTFontManagerScopeProcess, &error))
    {
        return false;
    }
    
    return true;
}

auto pFont::scale( unsigned pixel ) -> unsigned {
    
    return pixel;
}
    
auto pFont::getSizeFromString(std::string desc) -> unsigned {
    
    unsigned size = 0;
    std::vector<std::string> tokens = String::split(desc, ',');

    if(tokens.size() >= 2  && String::isNumber(tokens[1]))
        size = std::stoi( tokens[1] );

    return size;
}
    
auto pHelper::getColor(unsigned color) -> NSColor* {
    return [NSColor
         colorWithSRGBRed:((color>>16) & 0xff) / 255.0
         green:((color>>8) & 0xff) / 255.0
         blue:(color & 0xff) / 255.0
         alpha: 1.0];

}

auto pThreadPriority::setPriority( ThreadPriority::Mode mode, float typicalProcessingTimeInMilliSeconds, float maxProcessingTimeInMilliSeconds ) -> bool {

    [[NSThread currentThread] setThreadPriority: 1.0];
    kern_return_t result;
    mach_port_t machId = pthread_mach_thread_np( pthread_self() );

    thread_extended_policy_data_t extended;
    extended.timeshare = (mode == ThreadPriority::Mode::Normal) ? 1 : 0;
    result = thread_policy_set( machId, THREAD_EXTENDED_POLICY, (thread_policy_t)&extended, THREAD_EXTENDED_POLICY_COUNT);

    if (result != KERN_SUCCESS) {
        mach_error("thread policy error", result);
    }

    switch(mode) {
        default:
        case ThreadPriority::Mode::Normal: {
            thread_standard_policy_data_t standard;
            result = thread_policy_set( machId, THREAD_STANDARD_POLICY, (thread_policy_t)&standard, THREAD_STANDARD_POLICY_COUNT);
        } break;
        case ThreadPriority::Mode::High:
            thread_precedence_policy_data_t precedence;
            precedence.importance = 63;
            result = thread_policy_set( machId, THREAD_PRECEDENCE_POLICY, (thread_policy_t)&precedence, THREAD_PRECEDENCE_POLICY_COUNT);
            break;
        case ThreadPriority::Mode::Realtime: {
            mach_timebase_info_data_t tb_info;
            mach_timebase_info(&tb_info);
            double _clock = ((double)tb_info.denom / (double)tb_info.numer) * 1000.0;

            thread_time_constraint_policy_data_t timeConstraints;
            timeConstraints.period = 0;
            timeConstraints.computation = (unsigned)(typicalProcessingTimeInMilliSeconds * _clock * 1000.0);
            timeConstraints.constraint = (unsigned)(maxProcessingTimeInMilliSeconds * _clock * 1000.0);
            timeConstraints.preemptible = false;

            result = thread_policy_set( machId, THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&timeConstraints, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
        } break;
    }

    if (result != KERN_SUCCESS) {
        mach_error("thread policy error", result);
        return false;
    }

    return true;
}
    
}
