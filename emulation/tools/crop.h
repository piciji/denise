
#pragma once

#include <functional>
#include "../interface.h"

namespace Emulator {
	
template<typename T>
struct Crop {
    typedef Interface::CropType CropType;
	using BorderCallback = std::function<void( unsigned& top, unsigned& bottom, unsigned& left, unsigned& right )>;
	
	BorderCallback removeBorderCallback;
	BorderCallback monitorBorderCallback;
	
	struct {
		CropType type;
		bool aspectCorrect;
        bool ntsc;
		
		unsigned left; //or all directions
		unsigned right;
		unsigned top;
		unsigned bottom;
	} settings;
    
    struct Latest {
        T* frame;
        unsigned width;
        unsigned height;
        unsigned top;
        unsigned left;
        unsigned linePitch;
    } latest;
    
	unsigned croppedWidth;
	unsigned croppedHeight;
	
	unsigned top;
	unsigned bottom;
	unsigned left;
	unsigned right;
		
	auto updateBorder( ) -> bool {		
        
		if ( settings.type == CropType::Off ) {
            top = left = right = bottom = 0;
			return false;
		}
        
		if( settings.type == CropType::Auto ) {
			removeBorderCallback( top, bottom, left, right );
		
		} else if( settings.type == CropType::Monitor ) {		
			monitorBorderCallback( top, bottom, left, right );            
            
        } else if( settings.type == CropType::SemiAuto ) {
			top = settings.left;
			left = settings.left;
			right = settings.left;
			bottom = settings.left;
			
		} else if( settings.type == CropType::Free ) {			
			top = settings.top;
			left = settings.left;
			right = settings.right;
			bottom = settings.bottom;
		}
		return true;
	}
	
	auto apply(T*& frame, unsigned& width, unsigned& height, unsigned& linePitch) -> void {
		
		if ( !updateBorder() ) {
            memoryLatest( frame, width, height, linePitch );
			return;
		}
        
		unsigned lineLength = width + linePitch;
		
		croppedHeight = height - (top + bottom);
		croppedWidth = width - (left + right);

		if (settings.aspectCorrect &&
            (settings.type == CropType::Auto || settings.type == CropType::SemiAuto) )
			correct( width, height );

		frame += top * lineLength;
		frame += left;

		linePitch = lineLength - croppedWidth;
		
		height = croppedHeight;
		width = croppedWidth;
        
        memoryLatest( frame, width, height, linePitch );
	}
    
    auto memoryLatest( T*& frame, unsigned& width, unsigned& height, unsigned& linePitch ) -> void {
        // remember result of last crop for post processing ( e.g. lightgun )
        latest.frame = frame;
        latest.width = width;
        latest.height = height;
        latest.linePitch = linePitch;
        latest.top = top;
        latest.left = left;
    }
	
	auto roundUp( double value ) -> unsigned {

		if (value == 0)
			return 0;

		return (unsigned) ( value + 0.5 );
	}
	
	auto correct( unsigned width, unsigned height ) -> void {
		
		unsigned _correctedHeight = roundUp( (double)croppedWidth * (double)height / (double)width );
        unsigned diff;

		if (_correctedHeight >= croppedHeight) {

			diff = roundUp(((double) _correctedHeight - (double) croppedHeight) / 2.0);

            if (diff <= top ) top -= diff;
            else top = 0;

			croppedHeight = _correctedHeight;

			return;
		}
		
		unsigned _correctedWidth = roundUp( (double)croppedHeight * (double)width / (double)height );
		
		diff = roundUp( ( (double)_correctedWidth - (double)croppedWidth) / 2.0 );

        if (diff <= left ) left -= diff;
        else left = 0;

        croppedWidth = _correctedWidth;
	}
};

}