
#define NSAppKitVersionNumber10_4  824
#define NSAppKitVersionNumber10_5  949
#define NSAppKitVersionNumber10_6  1038
#define NSAppKitVersionNumber10_7  1138
#define NSAppKitVersionNumber10_8  1187
#define NSAppKitVersionNumber10_9  1265
#define NSAppKitVersionNumber10_10  1343
#define NSAppKitVersionNumber10_11  1404
#define NSAppKitVersionNumber10_12  1504
#define NSAppKitVersionNumber10_13  1561
#define NSAppKitVersionNumber10_14  1671
#define NSAppKitVersionNumber10_15  1700
#define NSAppKitVersionNumber11  2022
#define NSAppKitVersionNumber12  2113
#define NSAppKitVersionNumber13  2299
#define NSAppKitVersionNumber14  2487
#define NSAppKitVersionNumber15  2490
#define NSAppKitVersionNumber26  2500


auto isBigSur() -> bool {    
    return (NSAppKitVersionNumber >= NSAppKitVersionNumber11) && (NSAppKitVersionNumber < NSAppKitVersionNumber12);
}

auto hasMinimumVersion(unsigned major, unsigned minor) -> bool {
    
    unsigned _internalVersion = NSAppKitVersionNumber;
    
    if (major == 11) {
        switch(minor) {
            default:
            case 0: // Big Sur 11.0
                return _internalVersion >= NSAppKitVersionNumber11;

        }
    } else if (major == 12) {
        switch(minor) {
            default:
            case 0: // Monterey 12.0
                return _internalVersion >= NSAppKitVersionNumber12;

        }
    } else if (major == 13) {
        switch(minor) {
            default:
            case 0: // Ventura 13.0
                return _internalVersion >= NSAppKitVersionNumber13;
                
        }
    } else if (major == 14) {
        switch(minor) {
            default:
            case 0: // Sonoma 14.0
                return _internalVersion >= NSAppKitVersionNumber14;
                
        }
    } else if (major == 15) {
        switch(minor) {
            default:
            case 0: // Sequoia 15.0
                return _internalVersion >= NSAppKitVersionNumber15;
                
        }
    } else if (major == 26) {
        switch(minor) {
            default:
            case 0: // Tahoe 26.0
                return _internalVersion >= NSAppKitVersionNumber26;
                
        }
    } else if (major == 10) {
        
        switch(minor) {
            case 8: // Mountain Lion 10.8
                return _internalVersion >= NSAppKitVersionNumber10_8;
            case 9: // Mavericks 10.9
                return _internalVersion >= NSAppKitVersionNumber10_9;
            case 10: // Yosemite 10.10
                return _internalVersion >= NSAppKitVersionNumber10_10;
            case 11: // El Capitan 10.11
                return _internalVersion >= NSAppKitVersionNumber10_11;
            case 12: // Sierra 10.12
                return _internalVersion >= NSAppKitVersionNumber10_12;
            case 13: // High Sierra 10.13
                return _internalVersion >= NSAppKitVersionNumber10_13;
            case 14: // Mojave 10.14
                return _internalVersion >= NSAppKitVersionNumber10_14;
            case 15: // Catalina 10.15
                return _internalVersion >= NSAppKitVersionNumber10_15;
        }
    }
    return false;
}
