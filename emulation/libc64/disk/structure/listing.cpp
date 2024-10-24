
namespace LIBC64 {

// tracks: 1 - 40, sectors per track: 0 - 39
auto DiskStructure::getMfmPtr(unsigned _track, unsigned _sector) -> uint8_t* {
    if (_track == 0)
        return nullptr;

    unsigned _size = (_track - 1) * 40 * 256 + _sector * 256;
    if ((_size + 256) > rawSize)
        return nullptr;

    return rawData + _size;
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

    if ((ptr = getMfmPtr(_track, _sector++)) == nullptr)
        return;

    listings.push_back( { id++, listing.buildHeadline( ptr + 0x4, ptr + 0x19, ptr + 0x16 ), listing.decodeToScreencode( buildLoadCommand(_headlineCmd, true) ) } );
    loader.push_back( _headlineCmd );

    unsigned freeBlocks = 0;
    for (uint8_t t = 1; t <= 80; t++) {
        if (t == 1 || t == 41) {
            if ((ptr = getMfmPtr(_track, _sector++)) == nullptr)
                return;

            ptr += 0x10;
        }

        if (t != 40)
            freeBlocks += *ptr;
        ptr += 6;
    }

    if ((ptr = getMfmPtr(_track, _sector++)) == nullptr)
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

            if ((ptr = getMfmPtr(_track, _sector)) == nullptr)
                return;

            _track = ptr[0];
            _sector = ptr[1];
        }
    }

    listings.push_back( { id++, listing.buildFreeLine( freeBlocks ), listing.decodeToScreencode( buildLoadCommand( _headlineCmd, true) ) } );
    loader.push_back( _headlineCmd );
}

}
