
#ifndef STATUS_H
#define STATUS_H

#include <time.h>
#include "../audio/manager.h"
#include "../view/view.h"

struct Status {
	bool update;
	unsigned messageSecondsLeft;
	std::string message;
	unsigned fps, fpsCollect;
	time_t prev_t, curr_t;
	bool showFps;
	bool critical;
    bool record;
	
	struct DriveState {
        Emulator::Interface::Media* media;
		enum Mode : uint8_t { NoOperation = 0, Read = 1, Write = 2, List = 3, ReadHalf = 4, WriteHalf = 5, LedCart = 6 } mode;
        unsigned track;
    };
    std::vector<DriveState> driveStates;
	
	auto init(bool _update = false) -> void {
		update = _update;
		critical = false;
		message = "";
		messageSecondsLeft = 0;
		fps = fpsCollect = 0;
		showFps = globalSettings->get<bool>("fps", false);      
        record = false;
        driveStates.clear();
	}   
	
	auto updateDriveState(Emulator::Interface::Media* media, unsigned mode, unsigned track) -> void {
        if(!media)
            return;
        
		for (auto& driveState : driveStates) {
			if (driveState.media == media) {
				driveState.mode = (DriveState::Mode)mode;
				driveState.track = track;
				update = true;
				return;
			}
		}
		driveStates.push_back({media, (DriveState::Mode)mode, track });
	}
	
	auto countFrames() -> void {
		fpsCollect++;
		time(&curr_t);
        
		if (curr_t != prev_t) {
			fps = fpsCollect;
			fpsCollect = 0;
			if (messageSecondsLeft > 0) {
				messageSecondsLeft--;
			}
			update = true;
			
			if (!VideoManager::synchronized)
				// check input polling and message loop every 50 ms
				program->loopFrames = (fps * 50 ) / 1000;
			else
				// check input polling every frame
				program->loopFrames = 0;
		}
		prev_t = curr_t;
	}
	
	auto addMessage(std::string message, unsigned seconds = 3, bool critical = false) -> void {
		this->message = message;
		this->critical = critical;
		messageSecondsLeft = seconds;
		update = true;
	}
    
    auto clearMessage() -> void {
        this->message = "";
        messageSecondsLeft = 0;
        update = true;
    }
	
	auto show() -> void {
		if (!update)
            return;
        
		update = false;

		if (messageSecondsLeft != 0) {
			view->setStatusText(message, critical);
			return;
		}
        
		std::string out = "";
        std::string halfTrack = "";
        
		if (!program->isRunning) {
			view->setStatusText(out);
			return;
		}
        
		for (auto& driveState : driveStates) {
			if (!driveState.mode) 
				continue;
			
            if (!out.empty())
                out += " | ";            
            
            auto mediaGroup = driveState.media->group;
            
            if (mediaGroup->isExpansion())
                out += mediaGroup->name + ": ";
            else
                out += driveState.media->name + ": ";
            
			switch (driveState.mode) {
				case DriveState::Mode::Read:
                    out += "read ";
                    halfTrack = ( dynamic_cast<LIBC64::Interface*>(activeEmulator) && mediaGroup->isDisk() ) ? ".0" : "";
                    break;
				case DriveState::Mode::Write:
                    out += "write ";
                    halfTrack = ( dynamic_cast<LIBC64::Interface*>(activeEmulator) && mediaGroup->isDisk() ) ? ".0" : "";
                    break;
                case DriveState::Mode::ReadHalf:
                    out += "read ";
                    halfTrack = ".5";
                    break;
				case DriveState::Mode::WriteHalf:
                    out += "write ";
                    halfTrack = ".5";
                    break;
				case DriveState::Mode::List:
                    halfTrack = "";
                    break; // show track number permanently
                case DriveState::Mode::LedCart:
                    halfTrack = "";
                    out += "LED";
                    break;
			}
			
            if (!mediaGroup->isExpansion())
                out += std::to_string( driveState.track ) + halfTrack;
		}
                
		if (showFps) {
            if (!out.empty())
                out += " | ";   

			out += "fps: " + std::to_string(fps);            		
        }
        
        if (record) {
            if (!out.empty())
                out += " | ";

            out += "REC"; 
        }
        
        auto& drcS = audioManager->statistics;
        
        if (drcS.enable) {  
            if (!out.empty())
                out += " | ";   
            
            out += "DRC Puffer: " + GUIKIT::String::formatFloatingPoint( drcS.current, 2 ) + "% ";                        
            out += "[ " + GUIKIT::String::formatFloatingPoint( drcS.min, 2 ) + " : " + GUIKIT::String::formatFloatingPoint( drcS.max, 2 ) + " ]";
            out += " Ø " + GUIKIT::String::formatFloatingPoint( drcS.average, 2 ) + "%";
        }            

		view->setStatusText(out);
	}
};

extern Status* status;

#endif

