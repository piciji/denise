
namespace WDCFAMILY {

template<uint8_t actions> inline auto W65816::idle() -> void {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR
    SYNC();

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        if constexpr (actions & SET_FLAG_I)     p.i = true;
        if constexpr (actions & CLEAR_FLAG_I)   p.i = false;

        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
        SYNC();
    }
#endif
}

template<uint8_t actions> inline auto W65816::read(uint32_t addr) -> uint8_t {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR
    SYNC();

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        if constexpr (actions & SET_FLAG_I)     p.i = true;
        if constexpr (actions & CLEAR_FLAG_I)   p.i = false;

        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
        SYNC();
    }
#endif

    return READ_BYTE(addr);
}

template<uint8_t actions> inline auto W65816::write(uint32_t addr, uint8_t value) -> void {
    if constexpr (actions & SAMPLE_INTR)
        CHECK_INTR
    SYNC();

#ifdef SUPPORT_RDY
    while (lines & RDY_LINE) {
        if constexpr (actions & SAMPLE_INTR)
            CHECK_INTR
        SYNC();
    }
#endif

    WRITE_BYTE(addr, value);
}

template<uint8_t actions> inline auto W65816::readBank(uint32_t addr) -> uint8_t {
    return read<actions>( ((dbr << 16) + addr) & 0xffffff );
}

template<uint8_t actions> inline auto W65816::readPC() -> uint8_t {
    return read<actions>((pbr << 16) | pc++);
}

template<uint8_t actions> inline auto W65816::readStack(uint32_t addr) -> uint8_t {
    return read<actions>((s + addr) & 0xffff );
}

template<uint8_t actions> inline auto W65816::writeBank(uint32_t addr, uint8_t data) -> void {
    write<actions>( ((dbr << 16) + addr) & 0xffffff, data );
}

template<uint8_t actions> inline auto W65816::writeStack(uint32_t addr, uint8_t data) -> void {
    write<actions>((s + addr) & 0xffff, data );
}

template<uint8_t actions> auto W65816::push(uint8_t data) -> void {
    write<actions>(s, data);
    if constexpr (actions & NATIVE) s--;
    else { modeE ? decByteL(s) : (void)s--; }
}

template<uint8_t actions> auto W65816::pull() -> uint8_t {
    if constexpr (actions & NATIVE) s++;
    else { modeE ? incByteL(s) : (void)s++; }
    return read<actions>(s);
}

inline auto W65816::directAdr(uint32_t addr) -> uint32_t {
    if(modeE && ((d & 0xff) == 0) )
        return (d & 0xff00) | (addr & 0xff);

    return (d + addr) & 0xffff;
}

auto W65816::getDirectAddressIndirect(uint32_t offset) -> uint16_t {
    uint8_t lsb = read( directAdr(offset) );

    if(!modeE || ((d & 0xff) == 0))
        return (read( directAdr(offset + 1) ) << 8) | lsb;

    uint16_t addr = directAdr(offset + 1);

    if((addr & 0xff) == 0) // if +1 wraps page -> undo
        return (read((uint16_t)(addr - 0x100)) << 8) | lsb;

    return (read(addr) << 8) | lsb;
}

inline auto W65816::decByteL(uint16_t& reg) -> void {
    uint8_t byte = reg & 0xff;
    byte--;
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::incByteL(uint16_t& reg) -> void {
    uint8_t byte = reg & 0xff;
    byte++;
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::setByteL(uint16_t& reg, uint8_t byte) -> void {
    reg = (reg & 0xff00) | byte;
}

inline auto W65816::setByteH(uint16_t& reg, const uint8_t& byte) -> void {
    reg = (reg & 0x00ff) | (byte << 8);
}

}
