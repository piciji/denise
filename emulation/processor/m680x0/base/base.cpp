
#include "base.h"

namespace M68FAMILY {
	
enum : uint8_t { Byte, Word, Long };
template<> auto Base::clip<Byte>(uint32_t data) -> uint32_t { return data & 0xff; }
template<> auto Base::clip<Word>(uint32_t data) -> uint32_t { return data & 0xffff; }
template<> auto Base::clip<Long>(uint32_t data) -> uint32_t { return data & 0xffffffff; }

template<> auto Base::mask<Byte>() -> uint32_t { return 0xff; }
template<> auto Base::mask<Word>() -> uint32_t { return 0xffff; }
template<> auto Base::mask<Long>() -> uint32_t { return 0xffffffff; }

template<> auto Base::msb<Byte>() -> uint32_t { return 0x80; }
template<> auto Base::msb<Word>() -> uint32_t { return 0x8000; }
template<> auto Base::msb<Long>() -> uint32_t { return 0x80000000; }

template<> auto Base::sign<Byte>(uint32_t data) -> int32_t { return  (int8_t)data; }
template<> auto Base::sign<Word>(uint32_t data) -> int32_t { return (int16_t)data; }
template<> auto Base::sign<Long>(uint32_t data) -> int32_t { return (int32_t)data; }

template<> auto Base::bytes<Byte>() -> uint8_t { return 1; }
template<> auto Base::bytes<Word>() -> uint8_t { return 2; }
template<> auto Base::bytes<Long>() -> uint8_t { return 4; }

auto Base::parse(const char* s, uintmax_t sum) -> uintmax_t {
	return (
		*s == '0' || *s == '1' ? parse(s + 1, (sum << 1) | (*s - '0')) :
		*s == ' ' || *s == '_' ? parse(s + 1, sum) :
		*s ? parse(s + 1, sum << 1) :
		sum
	);
}
	
Base::Base() {
	for( uint8_t id : range(0xffff) ) opTable[id] = nullptr;
}

auto Base::setContext( M68Context* context ) -> void {
	ctxDefault = context;
	useDefaultContext();
}

auto Base::setInterruptType( uint8_t type ) -> void {
	this->interruptType = type;
}

auto Base::getCCR() -> uint8_t {
	return ctx->c << 0 | ctx->v << 1 | ctx->z << 2 | ctx->n << 3 | ctx->x << 4;
}

auto Base::setCCR(uint8_t data) -> void {
	ctx->c = data & 1;
	ctx->v = (data >> 1) & 1;
	ctx->z = (data >> 2) & 1;
	ctx->n = (data >> 3) & 1;
	ctx->x = (data >> 4) & 1;
}

auto Base::getSR() -> uint16_t {
	return getCCR() | (ctx->i & 7) << 8 | ctx->s << 13 | ctx->t << 15;
}

auto Base::setSR(uint16_t data) -> void {
	setCCR( uint8_t(data) );
	bool oldS = ctx->s;
	
	ctx->i = (data >> 8) & 7;
	ctx->s = (data >> 13) & 1;
	ctx->t = (data >> 15) & 1;
	
	if (oldS != ctx->s) { //mode change
		if (!ctx->s) { //switch to user
			ctx->ssp = ctx->a[7];	
			ctx->a[7] = ctx->usp;		
		}
	}
}

//interrupt acknowledge cycle
auto Base::getInterruptVector(uint8_t level) -> uint8_t {
    level &= 7;    

    switch ( interruptType ) {
        default:
        case AUTO_VECTOR: return 24 + level;
        case USER_VECTOR: return ctx->getUserVector( level ) & 0xff;
        case SPURIOUS: return 24; //external device responds with bus error during this cycle
        case UNINITIALIZED: return 15;
    }
}

auto Base::getIllegalVector(uint8_t pattern) -> uint8_t {
	if (pattern == 0xa) return 10;
	if (pattern == 0xf) return 11;
	return 4;
}

auto Base::prepareIllegalExceptions() -> void {
	for( uint16_t id : range(0x10000) ) 
		if (!opTable[id]) {			
			opTable[id] = [=] { 
				illegalException( getIllegalVector( id >> 12 ) );
			};
		}			
}

auto Base::getFunctionCodes() -> uint8_t {
	uint8_t code = ctx->s << 2;
	if (state.data)
		code |= 1;
	else
		code |= 1 << 1;
	
	return code;
}

auto Base::testCondition(uint8_t code) -> bool {
	switch( code & 0xf ) {
		case 0: return true;
		case 1: return false;
		case 2: return !ctx->c & !ctx->z;
		case 3: return ctx->c | ctx->z;
		case 4: return !ctx->c;
		case 5: return ctx->c;
		case 6: return !ctx->z;
		case 7: return ctx->z;
		case 8: return !ctx->v;
		case 9: return ctx->v;
		case 10: return !ctx->n;
		case 11: return ctx->n;
        case 12: return ctx->n == ctx->v;
        case 13: return ctx->n != ctx->v;
		case 14: return ctx->n == ctx->v && !ctx->z;
		case 15: return ctx->n != ctx->v || ctx->z;
	}
}

auto Base::sbcd(uint8_t src, uint8_t dest) -> uint8_t {
	uint16_t resLo = (dest & 0xf) - (src & 0xf) - ctx->x;
	uint16_t resHi = (dest & 0xf0) - (src & 0xf0);
    uint16_t result, tmp_result;
    result = tmp_result = resHi + resLo;
    int bcd = 0;
	if (resLo & 0xf0) {
		bcd = 6;
		result -= 6;
	}
	if (((dest - src - ctx->x) & 0x100) > 0xff) result -= 0x60;	
	ctx->c = ctx->x = ( (dest - src - bcd - ctx->x) & 0x300) > 0xff;
		
	if (clip<Byte>(result)) ctx->z = 0;
	ctx->n = negative<Byte>(result);
    ctx->v = ( ((tmp_result & 0x80) == 0x80) && ((result & 0x80) == 0) );
	return result;
}

auto Base::abcd(uint8_t src, uint8_t dest) -> uint8_t {
	uint16_t resLo = (src & 0xf) + (dest & 0xf) + ctx->x;
	uint16_t resHi = (src & 0xf0) + (dest & 0xf0);
    uint16_t result, tmp_result;
    result = tmp_result = resHi + resLo;
    if (resLo > 9) result += 6;
	ctx->x = ctx->c = (result & 0x3F0) > 0x90;
	if (ctx->c) result += 0x60;
	if (clip<Byte>(result)) ctx->z = 0;
	ctx->n = negative<Byte>(result);
    ctx->v = ( ((tmp_result & 0x80) == 0) && ((result & 0x80) == 0x80) );
	return result;
}

}