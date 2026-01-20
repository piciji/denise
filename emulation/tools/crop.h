
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
	std::function<Interface::Crop(unsigned _w, unsigned _h)> screenshotBorderCallback = nullptr;
	
	struct {
		CropType type;
        Interface::Crop crop;
	} settings;
    
    struct Latest {
        T* frame = nullptr; // important, if focus loss happens before any frame is repeatable
        unsigned width;
        unsigned height;
        unsigned top;
        unsigned left;
        unsigned linePitch;
        bool topLeftChanged;
    	uint8_t options;
    } latest;

	struct {
		unsigned width;
		unsigned height;
	} original;
    
	unsigned croppedWidth;
	unsigned croppedHeight;

	unsigned top;
	unsigned bottom;
	unsigned left;
	unsigned right;

	auto active() -> bool { return settings.type != CropType::Off; }

	auto updateBorder(uint8_t& options ) -> bool {
        
		if ( (options & 0x80) || (settings.type == CropType::Off) ) {
            top = left = right = bottom = 0;
			return false;
		}
        
		if( settings.type == CropType::Auto || settings.type == CropType::AutoRatio ) {
			removeBorderCallback( top, bottom, left, right );
		
		} else if( settings.type == CropType::Monitor ) {		
			monitorBorderCallback( top, bottom, left, right );            
            
        } else if( settings.type == CropType::AllSides || settings.type == CropType::AllSidesRatio ) {
			top = settings.crop.left;
			left = settings.crop.left;
			right = settings.crop.left;
			bottom = settings.crop.left;

            if (options & 3) {
                top <<= 1;
                bottom <<= 1;
            }
			if (options & 0xc) {
				unsigned _shift = (options & 0xc) >> 2;
				left <<= _shift;
				right <<= _shift;
            }
			
		} else if( settings.type == CropType::Free ) {			
			top = settings.crop.top;
			left = settings.crop.left;
			right = settings.crop.right;
			bottom = settings.crop.bottom;

            if (options & 3) {
                top <<= 1;
                bottom <<= 1;
            }
			if (options & 0xc) {
				unsigned _shift = (options & 0xc) >> 2;
				left <<= _shift;
				right <<= _shift;
			}
		}
		return true;
	}
	
	auto apply(T*& frame, unsigned& width, unsigned& height, unsigned& linePitch, uint8_t options = 0) -> void {		
		original.width = width;
		original.height = height;

		if ( !updateBorder(options) ) {
            memoryLatest( frame, width, height, linePitch, options );
			return;
		}
        
		unsigned lineLength = width + linePitch;
		
		croppedHeight = height - (top + bottom);
		croppedWidth = width - (left + right);

		if (settings.type == CropType::AutoRatio || settings.type == CropType::AllSidesRatio)
			correct( width, height, options );

		frame += top * lineLength;
		frame += left;

		linePitch = lineLength - croppedWidth;
		
		height = croppedHeight;
		width = croppedWidth;
        
        memoryLatest( frame, width, height, linePitch, options );
	}
    
    auto memoryLatest( T*& frame, unsigned& width, unsigned& height, unsigned& linePitch, uint8_t& options ) -> void {
        // remember result of last crop for post processing ( e.g. lightgun )
        latest.frame = frame;
        latest.width = width;
        latest.height = height;
        latest.linePitch = linePitch;
		latest.options = options & 0x7f;

        if ( (latest.top != top) || (latest.left != left) ) {
            latest.top = top;
            latest.left = left;
            latest.topLeftChanged = true;
        } else
            latest.topLeftChanged = false;
    }
	
	auto roundUp( double value ) -> unsigned {

		if (value == 0)
			return 0;

		return (unsigned) ( value + 0.5 );
	}
	
	auto correct( unsigned width, unsigned height, uint8_t& options ) -> void {
		bool lace = options & 3;
        bool hires = options & 0xc;

		unsigned _correctedHeight = roundUp( (double)croppedWidth * (double)height / (double)width );
        unsigned diff;

		if (_correctedHeight >= croppedHeight) {

			diff = roundUp(((double) _correctedHeight - (double) croppedHeight) / 2.0);

            if (diff <= top ) top -= diff;
            else top = 0;

			croppedHeight = _correctedHeight;

            if (lace) {
                top &= ~1;
                croppedHeight &= ~1;
            }

			return;
		}
		
		unsigned _correctedWidth = roundUp( (double)croppedHeight * (double)width / (double)height );
		
		diff = roundUp( ( (double)_correctedWidth - (double)croppedWidth) / 2.0 );

        if (diff <= left ) left -= diff;
        else left = 0;

        croppedWidth = _correctedWidth;

        if (hires) {
            left &= ~1;
            croppedWidth &= ~1;
        }
	}

	auto cropAlternatively(unsigned& width, unsigned& height, unsigned& pitch) -> uint8_t* {
		if (!screenshotBorderCallback)
			return nullptr;

		Emulator::Crop<T> _crop;
		_crop.settings.type = CropType::Free;
		_crop.settings.crop = screenshotBorderCallback(width, height);

		unsigned lineLength = latest.linePitch + latest.width;
		pitch = lineLength - original.width;
		width = original.width;
		height = original.height;

		T* frame = latest.frame;
		frame -= latest.left;
		frame -= latest.top * lineLength;

		_crop.apply(frame, width, height, pitch);

		return (uint8_t*)frame;
	}
};

}