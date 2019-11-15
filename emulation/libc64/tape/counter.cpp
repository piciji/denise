
#include <cmath>

#include "tape.h"

#define TAPE_FAST_PLAY_ROUNDS_PER_SECOND 4
#define TAPE_HEAD_SPEED 4.76e-2	// unit: m/s, not valid for fast forward / rewind
#define TAPE_RADIUS_TAKEUP_SPOOL_EMPTY 1.07e-2 // unit: m
#define TAPE_COUNTER_TO_SPOOL_RELATION 0.525 // always constant
// there are different thicknesses for 30, 60 or 90 min tapes
// 90 min tapes with the thickness of 30 min tapes wouldn't fit on the spools
// hence saving the same prg on a 30 min tape and a 90 min tape should result in
// different counter positions

// I wouldn't emulate 30, 60 or 90 min tapes, but rather tapes without size limit
// so we take one thickness value for all tape sizes (not entirely correct)
#define TAPE_RIBBON_THICKNESS 1.27e-5 // unit: m

#ifndef M_PI 
#define M_PI    3.14159265358979323846f 
#endif

// [tape counter]
// first we need a formula to transfer a position in virtual tape image 
// to datasette counter.
// the format of tape images consists of multiple cycle counts which means 
// time for zero-crossing from positive to negative voltage occurs.
// the amount of cycles between these crossings distinguish between 0 and 1
// for each readed bit. C64 cia will be informed when a voltage crossing happens
// but software decides if it's interpreted as bit 0 or 1.
// Over the datasette head a tape passes at constant speed because of
// the capstan drive controls the speed. The capstan drive has a constant radius
// and a constant angular velocity, so the speed is constant too.
// the take-up spool is driven by a motor too but it's job is to keep up the
// tension of the tape only
// so we start with the known to all formula v = s/t
// 's' is the length of a tape, we call it 'L' now
// 't' is the time to play a tape with a length of 'L'
// 'v' is the resulting speed at head
// the counter is driven by the take-up spool
// so we put in another well known formula for circumference
// C = 2 * Pi * r
// 'C' describes one revolution of the spool with an angle 'a' of 2*Pi
// 2Pi means 360 degree
// the length 'L' is the sum of the circumferences of each single revolution.
// of course we can use this formula to describe circumference for any angle.
// dL = da * r(a)   [d means delta]
// 'dL' means the length added to take-up spool when it rotates through an specific
// angle 'a' 
// the PROBLEM is that the speed is only constant at the head
// because of the radius of the take-up spool increases with the thickness of the tape
// in each revolution, means the speed of the take-up spool is not constant.
// ok we can say: when the radius increases by the thickness of tape, then the angle 
// increases of 2Pi ( a complete revolution)
// when dr ( delta radius ) = h (thickness of tape)
// then da ( delta angle ) = 2Pi
// proportion: dr / da = h / 2Pi
// dr = r1 - r0, r0 is not 0 for an empty take-up spool ( ~ 1 cm)
// da = a1 - a0, with a0 = 0 (starting angle for empty take-up spool )
// 'r1' and 'a1' describe radius and angle after some amount of time

// [substitute deltas and solve equation to r1]
// (r1-r0) / (a1 - 0) = h / 2Pi
// r1 = h/2Pi * a1 + r0
// [substitute r1 with formula above for circumference]
// dL / da = h/2Pi * a1 + r0 
// dL = (h/2Pi * a1 + r0) * da
// this is an integral: dL = f(a) * da  [between a0 = 0 and a1]
// the integral means calculation of an area: sum of all circumferences in this case
// remember: each circumference differ from another because of the increasing radius
// without the integral we had to calculate each single circumference which fits in 'L'
// to solve it we set for the initial condition 'a0' = 0 and 'L0' = 0, means empty take-up spool
// integration is the inverse of differentiation
// L = h / 4Pi * a^2 + r0 * a
// use first derivative to proof the integral: rule derivative of powers: r * x^(r-1)
// 2 * h / 4Pi * a^(2-1) + r0 * a^(1-1) = h/2Pi * a + r0 * a^0 = h/2Pi * a + r0 [proofed]
// ok back to the integral: L = h / 4Pi * a² + r0 * a
// remember our first formula: v = L/t
// now it's time to substitute L with v*t
// 'v' is the constant speed on tape head
// 't' is the elapsed time we want to find out the counter position
// furthermore we take into account, that a full revolution of the take-up spool doesn't mean
// a complete counter increment. there is some constant factor 'k' between both
// c = k*a
// we used a as radiants so far, 2Pi = 360 degree = 1 revolution
// now we want to count complete revolutions, so we simply divide by 2Pi
// c = k * a / 2Pi
// [substitute L and a]
// v*t = (h / 4Pi) * 4 * Pi^2 * c^2/k^2 + r0 * 2 * Pi * c/k
// [solve to t by dividing with v] 
// t = (c^2 * h * Pi)  / (v * k^2) + (c * r0 * 2 * Pi) / (v * k)  
// Now we can determine the elapsed time for tape counter 'c' by knowledge of the tape thickness 'h',
// the tape head speed 'v' and the initial radius 'r0' of the take-up spool
// ok we need to solve this equation to 'c' in order to get the counter value for a play time 't'
// there are a lot of constants in our formula, for simplicity we should combine some constants but
// leave the variables alone
// [simplify constants and insert them]
// a = (h * Pi) / (v * k^2)
// b = (r0 * 2 * Pi) / ( v * k )
// t = a * c^2 + b * c
// [we need to isolate c, so divide by a]
// t/a = c^2 + (b/a) * c
// [ we need to describe this term with one c only in order to solve the equation to 'c' ]
// t/a = (c + b/(2*a))^2 - b^2 / (4 * a^2)
// [proof it yourself by solving the quadratic term]
// [isolate the quadratic term in order to extract square root ]
// t/a + b^2 / (4 * a^2) = (c + b/(2*a))^2
// [square root this equation]
// sqrt( t/a + b^2 / (4 * a^2) ) = c + b/(2*a)
// [isolate c finaly]
// c = sqrt( t/a + b^2 / (4 * a^2) ) - b/(2*a)
// [insert substitution for a and b]
// c = sqrt( (t * v * k^2) / (h * Pi) +  (r0^2 * 4 * Pi^2 * v^2 * k^4 ) / ( 4 * v^2 * k^2 * h^2 * Pi^2 ) ) 
//      - (r0 * 2 * Pi * v * k^2) / ( 2 * v * k * h * Pi )
// [reduce]
// c = sqrt( (t * v * k^2) / (h * Pi) + (k^2 * r0^2) / h^2 ) - (k * r0) / h
// [exclude k^2]
// c = sqrt( k^2 * ( (t * v) / (h * Pi) + r0^2 / h^2 ) ) - (k * r0) / h
// [take root of k^2 to isolate it from overall root ]
// c = k * sqrt( (t * v) / (h * Pi) + r0^2 / h^2 ) - (k * r0) / h
// [isolate k]
// c = k * (sqrt( (t * v) / (h * Pi) + r0^2 / h^2 ) - r0 / h)
// thats it
// keep in mind using the same units for all variables/constants
// i.e. when speed is in meter / second
// then use meter for radius, thickness
// and use seconds for time

// [rewind + forward speed ]
// ok for both functions not the capstan drive regulate the speed but
// the take-up spool for 'forward' and the feed spool for 'rewind'
// means the head speed for these actions is not constant and vary 
// according to the radius of the spool
// for a realistic approach we have to recalculate the actual speed regularly
// ok from derivation above we catch the resulting formula after forming
// the integral to describe the sum of circumferences with different radiuses
// L = h / 4Pi * a^2 + r0 * a
// we have to isolate the angle 'a'
// [divide by h and multiply by 4Pi]
// (L * 4 * Pi) / h = a^2 + (4 * Pi * r0 * a) / h
// [we need to describe this term with one a only in order to solve the equation to 'a' ] 
// (L * 4 * Pi) / h = ( a + (2 * Pi * r0) / h )^2 - (4 * Pi^2 * r0^2) / h^2
// [isolate the quadratic term in order to extract square root ]
// (L * 4 * Pi) / h + (4 * Pi^2 * r0^2) / h^2 = (a + (2 * Pi * r0) / h)^2
// [square root this equation]
// sqrt( (L * 4 * Pi) / h + (4 * Pi^2 * r0^2) / h^2 ) = a + (2 * Pi * r0) / h
// [isolate a finaly]
// a = sqrt( (L * 4 * Pi) / h + (4 * Pi^2 * r0^2) / h^2 ) - (2 * Pi * r0) / h
// ok this step will be tricky, we need the tape thickness 'h' outside of the root
// you will see later why
// [exclude 1/h²]
// sqrt( (1 / h^2) * (L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) ) - (2 * Pi * r0) / h = a
// [take root of 1 / h^2 to isolate it from overall root ]
// sqrt( 1 / h^2 ) * sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) - (2 * Pi * r0) / h = a

// [ in detail ]
// sqrt( 1 / h^2 ) = sqrt ( h^-2 ) = (h^-2)^0.5 = h^( -2 * 0.5 ) = h^-1 = 1 / h

// 1 / h * sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) - (2 * Pi * r0) / h = a
// [exclude 1/h]
// 1 / h * (sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) - 2 * Pi * r0) = a
// [multiply with h]
// sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) - 2 * Pi * r0 = a * h
// [ + 2 * Pi * r0 ]
// sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) = a * h + 2 * Pi * r0
// ok from derivation above we reuse the formula:
// r1 = h/2Pi * a1 + r0
// which describes the change in radius for one revolution by the thickness 'h'
// [ multiply with 2 * Pi ]
// 2 * Pi * r1 = a1 * h + 2 * Pi * r0
// [insert this in our main formula]
// sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) = 2 * Pi * r1
// now we have a relation without the angle 'a'
// remember we want the speed at head for the rewind and forward functions in relation
// of a given radius r1 of the take-up spool or in case of rewind 'r1' of
// the feed spool
// the head speed is defined by the speed of an object traveling the circle at 'r1'
// the formula for the speed of an object traveling the circle: v = (2 * Pi * r) / T
// means: circumference divided by the time the circle needs for one revolution
// [inserting this in main formula we get]
// sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) = v1 * T
// [solve to v1]
// v1 = sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) / T
// T and therefore the angular velocity are constant for fast forward / rewind
// we can calculate the actual speed by knowing the thickness of the tape, the
// length of tape so far, the initial radius 'r0' and the elapsed time 'T' for
// one revolution

// so keep in mind for playing a cassette the speed on head is constant but not
// the angular velocity of the spools ( driven by capstan )
// for fast forward / rewind the angluar velocity of the spools are constant but not
// the speed on head ( driven by the spools)

namespace LIBC64 {

auto Tape::updateCounter() -> void {
    
    counter = ( 1000 - counterOffset + calculateCounter() ) % 1000;
	
    CounterState cstate;
    
    if (!motorIn) 
        cstate = CounterState::List;
    
    else {
        switch(mode) {
            default:
            case Mode::Forward:
            case Mode::Rewind:
            case Mode::Stop:    cstate = CounterState::List; break;
            case Mode::Play:    cstate = CounterState::Read; break;
            case Mode::Record:  cstate = CounterState::Write; break;
        }			
    }
    
    if ( currentCounter.counter == counter && currentCounter.cstate == cstate )
        return;
    
    currentCounter.counter = counter;
    currentCounter.cstate = cstate;
    
    updateState( cstate, counter );
}

auto Tape::resetCounter() -> void {
	
	counterOffset = calculateCounter() % 1000;
	
	updateCounter();
}

auto Tape::calculateCounter() -> unsigned {
    if (!loaded)
        return calculateCounterForNoTape();
	// formula is described in detail in the beginning of this file
    // c = k * (sqrt( (t * v) / (h * Pi) + r0^2 / h^2 ) - r0 / h)
    
    // precalculate some const values
    // this static stuff is calculated one time only for all instances of this class
	static double c1 = TAPE_HEAD_SPEED / (TAPE_RIBBON_THICKNESS * M_PI);
    
	static double c2 = (TAPE_RADIUS_TAKEUP_SPOOL_EMPTY * TAPE_RADIUS_TAKEUP_SPOOL_EMPTY) /
		(TAPE_RIBBON_THICKNESS * TAPE_RIBBON_THICKNESS);
    
	static double c3 = TAPE_RADIUS_TAKEUP_SPOOL_EMPTY / TAPE_RIBBON_THICKNESS;
    
    return TAPE_COUNTER_TO_SPOOL_RELATION * ( std::sqrt( cycles / cylcesPerSecond * c1 + c2 ) - c3 );
}

auto Tape::calculateCounterForNoTape() -> unsigned {
    // here we need the counter value for an elapsed amount of cycles when there is no cassette present
	// the speed for the spools are always constant because the radius doesn't change, doesn't matter
	// if play, fast forward or rewind
    // v = s / t 
	// 's' is the sum of all the circumferences of the empty spool which fits in the tape length passed
	// after 't' seconds 
    // v * t = s
	// s = revolutions * (2 * Pi * r)
    // v * t = revolutions * 2 * Pi * r
	// revolutions = ( v * t ) / ( 2 * Pi * r )
	// don't forget the constant factor beween spool and counter
	// c = revolutions * k
    
    return ((TAPE_HEAD_SPEED * cycles) / (cylcesPerSecond * 2 * M_PI * TAPE_RADIUS_TAKEUP_SPOOL_EMPTY )) * TAPE_COUNTER_TO_SPOOL_RELATION;                
}

auto Tape::setCyclesPerSecond( unsigned value ) -> void {
	
    if (cylcesPerSecond == value)
        return;
    
	cylcesPerSecond = value;    
    cycles = 0;
    
    // this is quite useless, we want to make the tape counter
    // working too when there is no cassette injected
    // for rewinding there is no limitation and the counter would run
    // forever 0, 999, 998 ... 0, 999, and so on
    // we use the amount of elapsed cycles to calculate the tape length
    // so we can not simple count -1, -2 ...
    // if the cycles fall below zero we have to use the cycle amount of
    // a counter value of 999 and decrease from there
    while(true) {
        cycles += 1000;
        // by brute force we get a rough calculated amount of cycles
        if (calculateCounterForNoTape() == 999) {
            cycles999 = cycles;
            break;
        }            
    }
    cycles = 0;
}

auto Tape::speedAdjustment() -> double {
	// formula is described in detail in the beginning of this file
	// v1 = sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 ) / T
	// 1 / T = f ( frequency )
	// v1 = f * sqrt( L * 4 * Pi * h + 4 * Pi^2 * r0^2 )
	// rewind and fast forward make 4 counter rounds per second
	// so f = 4, however we need the frequency of the spool, not the counter
	// we know the constant factor 'k' between counter and spool	
	// f = TAPE_FAST_PLAY_ROUNDS_PER_SECOND / TAPE_COUNTER_TO_SPOOL_RELATION
	// 'L' is the actual tape length: L = v * t
	// 'v' is the constant head speed in case of normal play operation
	// doesn't matter if we are in play, rewind or fast forward mode
	// we always know the amount of elapsed cycles to play a tape to actual
	// position ( amount of revolution counts on take-up spool )
	// now we need elapsed time from cycle counter
	// 1s = cycles per second ( pal or ntsc )
	// x  = actual cycles
	// [ solve to x ]
	// x = actual cycles / cycles per second
	// L = TAPE_HEAD_SPEED * cycles / cylcesPerSecond
	
	double speed = TAPE_HEAD_SPEED;
    
    static double c4 = 4 * M_PI * M_PI * TAPE_RADIUS_TAKEUP_SPOOL_EMPTY * TAPE_RADIUS_TAKEUP_SPOOL_EMPTY;
    static double c5 = 4 * M_PI * TAPE_RIBBON_THICKNESS * TAPE_HEAD_SPEED;
	static double c6 = 2 * M_PI * TAPE_RADIUS_TAKEUP_SPOOL_EMPTY;
    
    if ( mode == Mode::Play );
    
    else if ( !loaded ) {
		// if there is no cassette inserted the speed for fast forward and rewind is constant
		// because of the take-up spool or feed spool keep the initial radius
		// v = s / t 
		// 's' = 2 * Pi * r ( circumference of the empty spool  )
		// 't' is the time it needs for one revolution
		// we know that the counter takes 4 revolutions per second
		// and we know there is a constant factor 'k' between the spool and the counter
		// so we have the following proportion:
		// 1s = 4 / k revolutions
		// t  = 1 revolution		
		// t = k / 4 seconds for one revolution
		// v = s / k / 4 = s * 4 / k = (2 * Pi * r * 4 ) / k		
		speed = TAPE_FAST_PLAY_ROUNDS_PER_SECOND / TAPE_COUNTER_TO_SPOOL_RELATION * c6;
        
	} else if ( mode == Mode::Forward ) {
		
		speed = TAPE_FAST_PLAY_ROUNDS_PER_SECOND / TAPE_COUNTER_TO_SPOOL_RELATION
				* std::sqrt( c5 * cycles / cylcesPerSecond + c4);
        
	} else if ( mode == Mode::Rewind ) {
		// this time the motor drives the feed spool
		// for feed spool length calculation we need the remaining cycles instead of the
		// elapsed cycles
		// remaining cylces = total cycles - elapsed cycles
		speed = TAPE_FAST_PLAY_ROUNDS_PER_SECOND / TAPE_COUNTER_TO_SPOOL_RELATION
				* std::sqrt( c5 * (cyclesTotal - cycles) / cylcesPerSecond + c4);		
	}

	// the cycle amounts in the tape image are calculated for play operation
	// we need to adjust these cycle amounts for the actual forward/rewind speed 
	// yes forward/rewind speeds are not constant, see explanations above	
	// the relation is "INVERSE proportional", the slower the speed the more cycles
	// TAPE_HEAD_SPEED = cycles // speed at play operation
	// forward speed = x // proportional 
	// x = (TAPE_HEAD_SPEED / forward speed) * cycles // inverse proportional
	
	return TAPE_HEAD_SPEED / speed;
}

}

#undef TAPE_FAST_PLAY_ROUNDS_PER_SECOND
#undef TAPE_HEAD_SPEED
#undef TAPE_RADIUS_TAKEUP_SPOOL_EMPTY
#undef TAPE_COUNTER_TO_SPOOL_RELATION
#undef TAPE_RIBBON_THICKNESS
