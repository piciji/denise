
namespace LIBC64 {

// support for following DIR chain to second side (don't know if C64 DOS using this)
#define GTP(_T) \
    (sides == 1) ? &mTracks[0][(_T - 1) * 2] \
    : &mTracks[ (_T > TYPICAL_TRACKS) ? 1 : 0][ (((_T > TYPICAL_TRACKS) ? (_T - TYPICAL_TRACKS) : _T) - 1) * 2]

// C64 DOS (support 35 tracks per side)
auto DiskStructure::createListing() -> void {

    if (!rawData || (type == Type::Unknown))
        return;

    if (type == Type::D81 || type == Type::G81 || type == Type::P81)
        return createListingMfm();

    Emulator::C64Listing listing;
    listing.convertToScreencode = system->convertToScreencode;

    unsigned id = 0;

    uint8_t buffer[256];
    uint8_t _track = 18;
    uint8_t _sector = 0;
    int trackOffset = 0;
    uint8_t tries = TYPICAL_TRACKS * sides + 1;

    while (--tries) {
        decodeSector( GTP( _track ), buffer, _sector );
        uint8_t _trackLogical = buffer[0];

        if (_trackLogical == 18)
            break;

        if ((_trackLogical == 0) || (_trackLogical > (TYPICAL_TRACKS * sides))) {
            _track = getLogicalTrack(_track, 1);

        } else {
            trackOffset = _track - _trackLogical;

            _track = getLogicalTrack(_track, trackOffset);
        }
    }

    if (!tries) {
        trackOffset = 0;
        _track = 18;
        decodeSector( GTP( _track ), buffer, _sector );
    }

    unsigned freeBlocks = 0;

    // c64 DOS count free sectors for 35 tracks only
    for (uint8_t track = 1; track <= TYPICAL_TRACKS; track++) {

        uint8_t* bamPtr = &buffer[4 + 4 * (track - 1)];

        if (track != 18) {
            freeBlocks += *bamPtr;
        }
    }

    for (uint8_t track = 1; track <= 5; track++) { // Dolphin DOS 36 - 40
        uint8_t* bamPtr = &buffer[0xac + 4 * (track - 1)];
        freeBlocks += *bamPtr;
    }

    for (uint8_t track = 1; track <= 5; track++) { // Speed DOS 36 - 40
        uint8_t* bamPtr = &buffer[0xc0 + 4 * (track - 1)];
        freeBlocks += *bamPtr;
    }

    if (sides == 2) {
        for (uint8_t track = 1; track <= TYPICAL_TRACKS; track++) {
            freeBlocks += buffer[0xdd + (track - 1)];
        }
    }

    uint8_t buffer2[256];
    decodeSector( GTP( _track ), buffer2, ++_sector );
    uint8_t* ptr = &buffer2[0];

    bool addedHeadline = false;

    std::vector<uint8_t> _headlineCmd = {'*'};
    if (system->loadWithColumn)
        _headlineCmd = {':', '*'};

    unsigned entry = 0;

    while(1) {

        unsigned listingSize = *(ptr + 0x1f) * 256 + *(ptr + 0x1e);

        if ( *(ptr + 0x2) != 0 ) {

            if (!addedHeadline) {
                addedHeadline = true;
                uint8_t type = *(ptr + 0x2);

                if (system->loadWithColumn && ((type & 7) != 2) ) // when first file is not a PRG
                    _headlineCmd = {'*'};

                listings.push_back( { id++, listing.buildHeadline( buffer + 0x90, buffer + 0xa5, buffer + 0xa2 ), listing.decodeToScreencode( buildLoadCommand(_headlineCmd, true) ) } );
                loader.push_back( _headlineCmd );
            }

            uint8_t type = *(ptr + 0x2);
            std::vector<uint16_t> entry = listing.buildListing( ptr + 0x5, listingSize, type & ~0x30 );

            std::vector<uint16_t> loadCommand;

            if (listingSize)
                loadCommand = listing.decodeToScreencode( buildLoadCommand( listing.loader, true ) );

            listings.push_back( { id++, entry, loadCommand } );
			loader.push_back( listing.loader );
        }

        ptr += 0x20;
        entry++;

        if ((entry & 7) == 0) {

            // if the disk doesn't use the dir track, it could produce an endless loop
            if (entry > 250)
                break;

            _track = buffer2[0];
            _sector = buffer2[1];

            if (trackOffset)
                _track = getLogicalTrack(_track, trackOffset);

            if (_track > (TYPICAL_TRACKS * sides))
                break;

            if (_track == 0)
                break;

            if ( decodeSector(GTP( _track ), buffer2, _sector) != ERR_OK)
                break;

            ptr = &buffer2[0];
        }
    }

    if (!addedHeadline) {
        listings.push_back( { 0, listing.buildHeadline( buffer + 0x90, buffer + 0xa5, buffer + 0xa2 ) } );
        loader.push_back( {'*'} );
    }

    listings.push_back( { id++, listing.buildFreeLine( freeBlocks ), listing.decodeToScreencode( buildLoadCommand( _headlineCmd, true) ) } );
	loader.push_back( _headlineCmd );
}

// tracks: 1 - 80, sectors per track: 0 - 39
auto DiskStructure::getDecodedMfmLogical(unsigned _track, unsigned _sector) -> uint8_t* {
    static uint8_t buffer[512];

    if (_track == 0 || _track > 80)
        return nullptr;

    uint8_t _side = 1;
    // convert logical to physical sector
    if (_sector >= 20) {
        _side = 0;
        _sector -= 20;
    }

    auto _mTrack = getTrackPtr(_side, _track-1);
    if (type == Type::G81 || type == Type::P81) {
        if (!decodeSectorMfm( _mTrack, _track - 1, (_sector >> 1) + 1, &buffer[0]))
            return nullptr;
    } else { // D81
        if (!readSectorMfm( _mTrack, _track - 1, (_sector >> 1) + 1, &buffer[0]))
            return nullptr;
    }

    if (_sector & 1)
        return buffer + 256;
    return buffer;
}

auto DiskStructure::createListingMfm() -> void {
    Emulator::C64Listing listing;
    listing.convertToScreencode = system->convertToScreencode;

    unsigned id = 0;
    uint8_t* ptr;

    uint8_t _track = 40;
    uint8_t _sector = 0;

    std::vector<uint8_t> _headlineCmd = {'*'};
    if (system->loadWithColumn)
        _headlineCmd = {':', '*'};

    if ((ptr = getDecodedMfmLogical(_track, _sector++)) == nullptr)
        return;

    listings.push_back( { id++, listing.buildHeadline( ptr + 0x4, ptr + 0x19, ptr + 0x16 ), listing.decodeToScreencode( buildLoadCommand(_headlineCmd, true) ) } );
    loader.push_back( _headlineCmd );

    unsigned freeBlocks = 0;
    for (uint8_t t = 1; t <= 80; t++) {
        if (t == 1 || t == 41) {
            if ((ptr = getDecodedMfmLogical(_track, _sector++)) == nullptr)
                return;

            ptr += 0x10;
        }

        if (t != 40)
            freeBlocks += *ptr;
        ptr += 6;
    }

    if ((ptr = getDecodedMfmLogical(_track, _sector++)) == nullptr)
        return;

    _track = ptr[0];
    _sector = ptr[1];

    unsigned entry = 0;

    while(1) {
        unsigned listingSize = *(ptr + 0x1f) * 256 + *(ptr + 0x1e);

        if ( *(ptr + 0x2) != 0 ) {
            uint8_t type = *(ptr + 0x2);
            if (entry == 0 && system->loadWithColumn && ((type & 7) != 2) ) {
                _headlineCmd = {'*'};
                loader[0] = _headlineCmd;
            }

            std::vector<uint16_t> entry = listing.buildListing( ptr + 0x5, listingSize, type & ~0x30 );
            std::vector<uint16_t> loadCommand;

            if (listingSize)
                loadCommand = listing.decodeToScreencode( buildLoadCommand( listing.loader, true ) );

            listings.push_back( { id++, entry, loadCommand } );
            loader.push_back( listing.loader );
        }

        ptr += 0x20;
        entry++;

        if ((entry & 7) == 0) {
            if (entry > 250 || _track > 80 || _track == 0)
                break;

            if ((ptr = getDecodedMfmLogical(_track, _sector)) == nullptr)
                return;

            _track = ptr[0];
            _sector = ptr[1];
        }
    }

    listings.push_back( { id++, listing.buildFreeLine( freeBlocks ), listing.decodeToScreencode( buildLoadCommand( _headlineCmd, true) ) } );
    loader.push_back( _headlineCmd );
}

}
