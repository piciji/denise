
#pragma once

/**
 * the interpolation is described by a cubic polynom f(x) = ax^3 + bx^2 + cx + d
 * 
 * Rules:
 * you need 4 points p0, p1, p2, p3 but interpolation happens only between p1 and p2
 * p0 and p3 are needed to calculate the slope in p1 and p2, so you can solve f(x) for specific points between p1 and p2
 * 
 * following is valid only for p1 and p2
 * f'(p1.x) = k1 = (p2.y - p0.y) / (p2.x - p0.x) // based on linear slope between p0 and p2
 * f'(p2.x) = k2 = (p3.y - p1.y) / (p3.x - p1.x) // based on linear slope between p1 and p3
 * 
 * linear equations in matrix view ( i = P1, j = P2)
 *
 *  [ 1  xi    xi^2    xi^3 ]   [ d ]    [ yi ]
 *  [     1  2*xi    3*xi^2 ] * [ c ] =  [ ki ] f'(x) slope  
 *  [ 1  xj    xj^2    xj^3 ]   [ b ]    [ yj ]
 *  [     1  2*xj    3*xj^2 ]   [ a ]    [ kj ] f'(x) slope
 *
 * NOTE: dy = yj - yi, dx = xj - xi
 * releasing c and d can be done in one step easily but in dependancy of (a) and (b)
 * 
 * c = ki - (3*xi*a + 2*b)*xi		alternate: c_alt = kj - (3*xj*a + 2*b)*xj
 * d = yi - ((xi*a + b)*xi + c)*xi  alternate: d_alt = yj - ((xj*a + b)*xj + c)*xj
 * 
 * now we want to get a term for (a) (step by step)
 * -------------------------------------------------- 
 * 
 * [ set d = d_alt ]
 * yj - xj*c - xj^2*b - xj^3*a = yi - xi*c - xi^2*b - xi^3*a
 * [ substitute term for c on left site and c_alt on right ]
 * yj - xj*( ki - 2*xi*b - 3*xi^2*a ) - xj^2*b - xj^3*a = yi - xi*( kj - 2*xj*b - 3*xj^2*a ) - xi^2*b - xi^3*a
 * [ multiply ]
 * yj - xj*ki + 2*xj*xi*b + 3*xj*xi^2*a - xj^2*b - xj^3*a = yi - xi*kj + 2*xi*xj*b + 3*xi*xj^2*a - xi^2*b - xi^3*a
 * [ simplify ]
 * yj - xj*ki + 3*xj*xi^2*a - xj^2*b - xj^3*a = yi - xi*kj + 3*xi*xj^2*a - xi^2*b - xi^3*a
 * 
 * [ we need some variance for gauss elimination ... so this time substitute term for c_alt on left site and c on right ]
 * 
 * yj - xj*( kj - 2*xj*b - 3*xj^2*a ) - xj^2*b - xj^3*a = yi - xi*( ki - 2*xi*b - 3*xi^2*a ) - xi^2*b - xi^3*a
 * [ multiply ]
 * yj - xj*kj + 2*xj^2*b + 3*xj^3*a - xj^2*b - xj^3*a = yi - xi*ki + 2*xi^2*b + 3*xi^3*a - xi^2*b - xi^3*a
 * [ simplify ]
 * yj - xj*kj + xj^2*b + 2*xj^3*a = yi - xi*ki + xi^2*b + 2*xi^3*a
 * 
 * now we add both resulting equations together and get rid of variable b [gauss elimination]
 * 
 * 2*yj - xj*ki + 3*xj*xi^2*a + xj^3*a - xj*kj = 2*yi - xi*kj + 3*xi*xj^2*a + xi^3*a - xi*ki
 *
 * [simplify]
 * 
 * 3*xj*xi^2*a + xj^3*a - 3*xi*xj^2*a - xi^3*a = 2*yi - 2*yj - xi*kj - xi*ki + xj*ki + xj*kj // put (a) sub terms on left side
 * 
 * a( 3xi^2*xj + xj^3 - 3xj^2*xi - xi^3 ) // isolate (a), then disassemble with polynom division (xj-xi)^3
 * 
 * a ( xj - xi )^2 * (xj-xi) + 2*yj - 2*yi = xj*ki + xj*kj - xi*kj - xi*ki
 * 
 * a ( xj - xi )^2 * (xj-xi) + 2*yj - 2*yi = ki ( xj-xi ) + kj (xj - xi ) // divide by (xj-xi)
 * 
 * a ( xj - xi )^2 + 2 (yj - yi) / (xj-xi) = ki + kj
 * 
 * a ( xj - xi )^2 = (ki + kj) - 2 (yj - yi) / (xj-xi)
 * 
 * a = ( (ki + kj) - 2 (yj - yi) / (xj - xi) ) / ( xj - xi )^2 // no dependancy on (b) (c) and (d)
 * 
 * finally we need a term for (b), todo: step by step derivation
 * 
 * b = ((kj - ki) / (xj-xi) - 3*(xi + xj) * a) / 2
 * 
 * -----------------------------------------------  
 * saddle point (minimax point): f''(x) = 0
 * -----------------------------------------------
 * [ building f''(x) use the same rules like f'(x) ]
 * 2 * b + 6 * a * xi = 0 
 * [ insert term for (b) from above ]
 * ( 2 * ( ( (kj - ki) / dx ) - 3 * a * (xi + xj) ) / 2 ) + 6 * a * xi = 0 // divide by (a)
 * 
 * ( (kj - ki) / (dx * a) ) - 3 * (xi + xj) + 6 * xi = 0
 * 
 * ( (kj - ki) / (dx * a) ) - 3xi - 3xj + 6xi = 0
 * 
 * (kj - ki) / (dx * a) = 3 dx // multiply dx * a and finaly substitue (a)
 * 
 * kj - ki = 3 dx^2 * ( ((ki + kj) - 2*dy/dx)/(dx*dx) ) 
 * 
 * kj - ki = 3 ( ki + kj - (2 dy / dx) )
 * 
 * kj - ki = 3ki + 3kj - 6 dy / dx
 * 
 * -4ki = 2kj - 6 dy / dx
 * 
 * -2ki = kj - 3 dy / dx
 * 
 * ki = (3 (dy / dx) - kj ) / 2
 */


#include <vector>
namespace Emulator {
     	
struct Coordinate {
    double x;
    double y;
};    

template<class T = uint16_t>    
struct PointPlotter {

	PointPlotter(T* result) : result(result) {}

	void operator()(double x, double y) {
		if (y < 0)
			y = 0;

		result[ int(x) ] = T(y + 0.5);
	}
	
protected:
	T* result;
};
    
struct CatmullSpline {
       
	template<class PointPlotter>
    static auto interpolate( std::vector<Coordinate>& coordinates, PointPlotter plot, double res ) -> void {
        
		unsigned size = coordinates.size();
        unsigned pos = 0;
		double k1, k2;
		
		if (size < 4)
			return;
        
        Coordinate* c0 = &coordinates[pos++];
        Coordinate* c1 = &coordinates[pos++];
        Coordinate* c2 = &coordinates[pos++];
        Coordinate* c3 = &coordinates[pos++];
        
        while( true ) {

            if (c1->x == c2->x) // single point
                continue;
            
            if (c0->x == c1->x && c2->x == c3->x)
				k1 = k2 = (c2->y - c1->y) / (c2->x - c1->x);
			
			// p0 and p1 equal; use f''(x1) = 0 (saddle point)
			else if ( c0->x == c1->x ) {
				k2 = (c3->y - c1->y) / (c3->x - c1->x);
				k1 = (3 * ( (c2->y - c1->y) / (c2->x - c1->x) ) - k2 ) / 2;
			// p2 and p3 equal; use f''(x2) = 0 (saddle point)
			} else if ( c2->x == c3->x ) {
				k1 = (c2->y - c0->y) / (c2->x - c0->x);
				k2 = (3 * ( (c2->y - c1->y) / (c2->x - c1->x) ) - k1 ) / 2;
			} else { // normal
				k1 = (c2->y - c0->y) / (c2->x - c0->x);
				k2 = (c3->y - c1->y) / (c3->x - c1->x);				
			}				            

			solve( c1->x, c1->y, c2->x, c2->y, k1, k2, res, plot );
			
            if ( pos == size )
                break;
            
            c0 = c1;
            c1 = c2;
            c2 = c3;
            c3 = &coordinates[pos++];
        }
    }
	
protected:
	template<class PointPlotter>
	static auto solve( double x1, double y1, double x2, double y2, double k1, double k2, double res, PointPlotter& plot ) -> void {

		double dx = x2 - x1, dy = y2 - y1;

		double a = ((k1 + k2) - 2 * dy / dx) / (dx * dx);
		double b = ((k2 - k1) / dx - 3 * (x1 + x2) * a) / 2;
		double c = k1 - (3 * x1 * a + 2 * b) * x1;
		double d = y1 - ((x1 * a + b) * x1 + c) * x1;

		for (double x = x1; x <= x2; x += res) {
			//simply solve f(x) for the coordinates between P1 and P2
			double y = ((a * x + b) * x + c) * x + d;
			plot(x, y);
		}
	}
};   
    
}