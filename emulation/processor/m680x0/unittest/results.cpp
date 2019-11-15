
#include "results.h"
#include "../base/range.h"

#include <assert.h>

namespace M68FAMILY {	
	
std::vector<Results*> Results::list;	
	
Results::Results(std::string ident, bool add) {
	this->ident = ident;	
	
	if (add) { //samples
		Results::add( this );		
		ctx = new M68Context;		
		resetSR();
		this->irqChecked = true; //expect all opcodes check for incomming irqs
		ctx->ird = 0xffff; //expect for next opcode
	}
}

Results::~Results() {	
	if (this->ident == "") return; //control ctx, memory deallocation extern
	
	if (Results::get( this->ident ) != nullptr) {
		if (memory) delete memory;
		if (ctx) delete ctx;		
	}
}

auto Results::compare( Results* a, Results* b, std::string& error ) -> bool {
	//show first error found only
	if (a->illegalException != b->illegalException) {
		error = "illegal exception: " + std::to_string( a->illegalException ) + " -> "
					+ std::to_string( b->illegalException );
		return false;
	}
	
	if (a->privilegeException != b->privilegeException) {
		error = "privilege exception: " + std::to_string( a->privilegeException ) + " -> "
					+ std::to_string( b->privilegeException );
		return false;
	}

	if (a->addressError != b->addressError) {
		error = "address error: " + std::to_string( a->addressError ) + " -> "
					+ std::to_string( b->addressError );
		return false;
	}

	if (a->busError != b->busError) {
		error = "bus error: " + std::to_string( a->busError ) + " -> "
					+ std::to_string( b->busError );
		return false;
	}

	if (a->group2Exception != b->group2Exception) {
		error = "group 2 exception: " + std::to_string( a->group2Exception ) + " -> "
					+ std::to_string( b->group2Exception );
		return false;
	}

	for(auto i : range(8)) {
		if (a->ctx->a[i] != b->ctx->a[i]) {
			error = "reg A" + std::to_string(i) + ": "
					+ Results::toHex(a->ctx->a[i]) + " -> "
					+ Results::toHex(b->ctx->a[i]);
			return false;
		}
		
		if (a->ctx->d[i] != b->ctx->d[i]) {
			error = "reg D" + std::to_string(i) + ": "
					+ Results::toHex(a->ctx->d[i]) + " -> "
					+ Results::toHex(b->ctx->d[i]);
			return false;
		}		
	}
	
	if (a->ctx->z != b->ctx->z) {
		error = "flag Z: " + std::to_string( a->ctx->z ) + " -> "
				+ std::to_string( b->ctx->z );
		return false;
	}
	if (a->ctx->n != b->ctx->n) {
		error = "flag N: " + std::to_string( a->ctx->n ) + " -> "
				+ std::to_string( b->ctx->n );
		return false;
	}
	if (a->ctx->x != b->ctx->x) {
		error = "flag X: " + std::to_string( a->ctx->x ) + " -> "
				+ std::to_string( b->ctx->x );
		return false;
	}
	if (a->ctx->v != b->ctx->v) {
		error = "flag V: " + std::to_string( a->ctx->v ) + " -> "
				+ std::to_string( b->ctx->v );
		return false;
	}
	if (a->ctx->c != b->ctx->c) {
		error = "flag C: " + std::to_string( a->ctx->c ) + " -> "
				+ std::to_string( b->ctx->c );
		return false;
	}

	if (a->ctx->i != b->ctx->i) {
		error = "i-mask: " + std::to_string( a->ctx->i ) + " -> "
				+ std::to_string( b->ctx->i );
		return false;
	}

	if (a->ctx->s != b->ctx->s) {
		error = "supervisor: " + std::to_string( a->ctx->s ) + " -> "
				+ std::to_string( b->ctx->s );
		return false;
	}
	
	if (a->ctx->t != b->ctx->t) {
		error = "trace: " + std::to_string( a->ctx->t ) + " -> "
				+ std::to_string( b->ctx->t );
		return false;
	}
	

	if (!Results::compareMemory(a->memory, b->memory, error)) {
		return false;
	}

	if (!Results::compareMemory(b->memory, a->memory, error)) {
		return false;
	}
	
	if (a->ctx->ird != b->ctx->ird) {
		error = "ird: " + Results::toHex(a->ctx->ird) + " -> "
			+ Results::toHex(b->ctx->ird);
		return false;
	}
	
	if (a->cycles != b->cycles) {
		error = "cycles: " + std::to_string( a->cycles ) + " -> "
			+ std::to_string( b->cycles );			
		return false;
	}
	
	if (a->irqChecked != b->irqChecked) {
		error = "irq not checked";
		return false;
	}
	
	return true;
}

auto Results::compareMemory(Memory* a, Memory* b, std::string& error) -> bool {
	
	if ( !Memory::isEqual(a, b, 30) ) {
		
		if (!b->faultCell) {
			error = "no match for address " + Results::toHex(a->faultCell->address);
		} else {
			error = "address " + Results::toHex(a->faultCell->address) + ": "
					+ Results::toHex(a->faultCell->value) + " -> "
					+ Results::toHex(b->faultCell->value);
		}
		return false;
	}	
	return true;
}

auto Results::toHex(int value) -> std::string {

	std::string out = "0x";
	
    char hex[8];

    sprintf( hex, "%x", value );
    
    out += (std::string)hex;
    	
	return out;
}

auto Results::add(Results* result) -> void {
    if (result->ident == "")
        return;

    if (Results::get( result->ident ) != nullptr) {
		assert( false ); //ident is already in use
    }

    list.push_back( result );
}

auto Results::get(std::string ident) -> Results* {
    for(auto obj : list) {
        if (obj->ident == ident)
            return obj;
    }
	
    return nullptr;
}

}