
#ifndef LOGGER_H
#define LOGGER_H

#include <vector>
#include <string>
#include "../../guikit/api.h"

struct Logger {	        

	auto log(std::string data, bool newLine = true) -> void {
		
		if ( newLine ) {						
			if ( bufferSize && (bufferSize == lines.size()) ) {
				write();
			}				
		}

		if ( newLine || ( lines.size() == 0 ) ) {
			lines.push_back( data );
		} else {
			lines[ lines.size() - 1 ] += " " + data;
		}
	}

	auto write() -> void {
		if (lines.size() == 0)
            return;
		
		if (!file) {
			if (savePath.empty())
                return;
			
			file = new GUIKIT::File(savePath + "log.txt");
			
			if( !file->open(GUIKIT::File::Mode::Write, true) ) {
                close();
                return;
            }                
		}				

		auto fp = file->getHandle();
		std::string out;

		for (unsigned i = 0; i < lines.size(); i++) {
			out = lines[ i ] + "\n";
			fputs( out.c_str(), fp );
		}		
		lines.clear();
	}

	auto setSavePath(std::string path) -> void {
		savePath = path;
		if (file) delete file;
		file = nullptr;
	}
	auto setBufferSize(unsigned size) -> void {
		bufferSize = size;
	}
    
    auto close() -> void {
        if (file)
            delete file;
        
        file = nullptr;
    }
	
	Logger() {
		bufferSize = 1000;
	}
	virtual ~Logger() {
		write();
		close();
	}

private:
	GUIKIT::File* file = nullptr;
	std::vector<std::string> lines;
	unsigned bufferSize;
	std::string savePath;
};

extern Logger* logger;

#endif

