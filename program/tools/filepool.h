
#ifndef FILEPOOL_H
#define FILEPOOL_H

struct FilePool {
	
	struct FilePtr {
		GUIKIT::File* file;
		std::string ident;
	};
	
	std::vector<FilePtr> filePtrs;	
	std::vector<GUIKIT::File> files;

    auto has(std::string ident, GUIKIT::File* file) -> bool {
        
        FilePtr* filePtr = find( ident );
        
        if (!filePtr)
            return false;
        
        return filePtr->file == file;
    }
    
	auto assign(std::string ident, GUIKIT::File* file) -> void {		
		FilePtr* filePtr = find( ident );
		
		if ( !filePtr )			
			filePtrs.push_back( { file, ident } );	
		else
			filePtr->file = file;	        
	}
	
	auto find(std::string ident) -> FilePtr* {
		for(auto& filePtr : filePtrs)
			if (filePtr.ident == ident)
				return &filePtr;
	
		return nullptr;
	}
    
    auto isAssigned( std::string filePath, std::string ident ) -> bool {
        
        FilePtr* filePtr = find( ident );
        
        if (!filePtr || !filePtr->file)
            return false;
        
        return filePtr->file->getFile() == filePath;
    }
		
	auto get(std::string filePath, bool createIfNotExists = true) -> GUIKIT::File* {
		GUIKIT::File* assignFile = nullptr;
		if (filePath.empty()) return nullptr;

		for (auto& file : files) {
			if (file.getFile() == filePath)
				return &file;
			
			if ((assignFile == nullptr) && (file.getFile().empty()))
				assignFile = &file;
		}
        
        if (!createIfNotExists)
            return nullptr;
        
		if (!assignFile) {
			files.push_back({});
			assignFile = &files.back();
		}
		
		assignFile->setFile(filePath);
		return assignFile;
	}
	   
    auto unloadOrphaned() -> void {
		for (auto& file : files) {
			bool referenceFound = false;

			for(auto& filePtr : filePtrs) {
				if (!filePtr.file)
                    continue;				

				if (&file == filePtr.file) {
					referenceFound = true;
					break;				
				}
			}
			if (!referenceFound)
				file.unload();
		}
	}
	
	FilePool( unsigned slots ) {
		for(unsigned i = 0; i < slots; i++)
			files.push_back({});
	}
	
	~FilePool() {
		for (auto& file : files)
			file.unload();
	}	
};

extern FilePool* filePool;

#endif 

