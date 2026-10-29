
#pragma once

struct FilePool {
	
	struct FilePtr {
		GUIKIT::File* file;
		std::string ident;
	};
	
	std::vector<FilePtr> filePtrs;	
	std::vector<GUIKIT::File*> files;

    auto has(const std::string& ident, GUIKIT::File* file) -> bool {
        
        FilePtr* filePtr = find( ident );
        
        if (!filePtr)
            return false;
        
        return filePtr->file == file;
    }
    
	auto assign(const std::string& ident, GUIKIT::File* file) -> void {
		FilePtr* filePtr = find( ident );
		
		if ( !filePtr )
			filePtrs.push_back( { file, ident } );
		else
			filePtr->file = file;
	}
	
	auto find(const std::string& ident) -> FilePtr* {
		for(auto& filePtr : filePtrs)
			if (filePtr.ident == ident)
				return &filePtr;
	
		return nullptr;
	}
    
    auto isAssigned( const std::string& filePath, const std::string& ident ) -> bool {
        
        FilePtr* filePtr = find( ident );
        
        if (!filePtr || !filePtr->file)
            return false;
        
        return filePtr->file->getFile() == filePath;
    }
		
	auto get(const std::string& filePath, bool createIfNotExists = true) -> GUIKIT::File* {
		GUIKIT::File* assignFile = nullptr;
		if (filePath.empty()) return nullptr;

		for (auto* file : files) {
			if (file->getFile() == filePath)
				return file;
			
			if ((assignFile == nullptr) && (file->getFile().empty()))
				assignFile = file;
		}
        
        if (!createIfNotExists)
            return nullptr;
        
		if (!assignFile) {
		    assignFile = new GUIKIT::File;
			files.push_back(assignFile);
		}
		
		assignFile->setFile(filePath);
		return assignFile;
	}
	   
    auto unloadOrphaned() -> void {
		for (auto* file : files) {
			bool referenceFound = false;

			for(auto& filePtr : filePtrs) {
				if (!filePtr.file)
                    continue;				

				if (file == filePtr.file) {
					referenceFound = true;
					break;				
				}
			}
			if (!referenceFound)
				file->unload();
		}
	}
	
	FilePool( unsigned slots ) {
        files.reserve(slots);
	}
	
	~FilePool() {
		for (auto* file : files)
		    delete file;

        files.clear();
	}	
};

extern FilePool* filePool;
