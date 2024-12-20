
namespace WDCFAMILY {

inline auto W65816::read(uint32_t addr) -> uint8_t {
    return REF_CALL READ_BYTE(addr);
}

inline auto W65816::readBank(uint32_t addr) -> uint8_t {
    return read( ((dbr << 16) + addr) & 0xffffff );
}

inline auto W65816::readPC() -> uint8_t {
    return read((pbr << 16) | pc++);
}

inline auto W65816::readStack(uint32_t addr) -> uint8_t {
    return read((s + addr) & 0xffff );
}

inline auto W65816::write(uint32_t addr, uint8_t value) -> void {
    REF_CALL WRITE_BYTE(addr, value);
}

inline auto W65816::writeBank(uint32_t addr, uint8_t data) -> void {
    write( ((dbr << 16) + addr) & 0xffffff, data );
}

inline auto W65816::writeStack(uint32_t addr, uint8_t data) -> void {
    write((s + addr) & 0xffff, data );
}

template<bool native> auto W65816::push(uint8_t data) -> void {
    write(s, data);
    if constexpr (native) s--;
    else { modeE ? decByteL(s) : (void)s--; }
}

template<bool native> auto W65816::pull() -> uint8_t {
    if constexpr (native) s++;
    else { modeE ? incByteL(s) : (void)s++; }
    return read(s);
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
