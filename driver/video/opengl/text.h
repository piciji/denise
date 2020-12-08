
#include "../../tools/utf8.h"

struct OpenGLText {
    
    FT_Library ft = nullptr;
    FT_Face face = nullptr;
    FT_GlyphSlot glyph;
    GLuint vao;
    GLuint vbo;
    
    GLuint program = 0;
    GLuint vertex = 0;
    GLuint fragment = 0;
    
    GLuint textTex = 0;
    
    FT_Byte* data = nullptr;

    enum {
        ALIGN_LEFT/*Top*/ = 0,  ALIGN_CENTER = 1, ALIGN_RIGHT = 2, VALIGN_BOTTOM = 4, VALIGN_CENTER = 16
    };
        
    unsigned char* textBuffer = nullptr;
    unsigned totalWidth = 0;
    unsigned totalHeight = 0;
    
    bool initialized = false;
    bool disable = false;
    
    OpenGLText() {
    }
    
    ~OpenGLText() {
        term();
    }
    
    auto setFontSize(int value) -> void {
        FT_Set_Pixel_Sizes(face, 0, value);
    }
    
    auto setColor(float r, float g, float b, float a) -> void {
        if (program) {
            glUseProgram(program);
            glUniform4f(glGetUniformLocation(program, "color"), r, g, b, a);
        }            
    }
    
    auto term() -> void {
        
        if(face) {
            FT_Done_Face(face);
            face = nullptr;
        }
        
        if(ft) {
            FT_Done_FreeType(ft);
            ft = nullptr;
        }
        
        if (vertex) {
            glDetachShader(program, vertex);
            glDeleteShader(vertex);
            vertex = 0;
        }
        
        if (fragment) {
            glDetachShader(program, fragment);
            glDeleteShader(fragment);
            fragment = 0;
        }
        
        if (program) {
            glDeleteProgram(program);
            program = 0;
        }

        if (vbo) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        
        if (vao) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        
        if (textBuffer) {
            delete[] textBuffer;
            textBuffer = nullptr;
        }
        
        if (textTex) {
            glDeleteTextures(1, &textTex);
            textTex = 0;            
        }
        
        if (data) {
            delete[] data;
            data = nullptr;
        }
        
        initialized = false;
    }
    
    auto getFontPath() -> std::string {
        
    #ifdef _WIN32
        #include <cstdlib>
        std::string winDir = "C:/Windows"; // fallback: most likely
        
        const char* env = std::getenv("WINDIR");        
        if (env)
            winDir = env;
                
        return winDir + "/Fonts/ARIALUNI.TTF";
    #elif __APPLE__
        return "/Library/Fonts/LucidaGrande.ttf";
    #else
        return "/usr/share/fonts/truetype/arial.ttf";
    #endif
    }
    
    auto init() -> bool {
        term();
        #include "../../tools/fonts.c"
        
        if (FT_Init_FreeType( &ft ))
            return false;
        
        if (FT_New_Face(ft, getFontPath().c_str(), 0, &face)) {
            data = new FT_Byte[ sizeof (sourceCodePro) ];
            std::memcpy(data, &sourceCodePro, sizeof (sourceCodePro));
            
            if (FT_New_Memory_Face(ft, data, sizeof(sourceCodePro), 0, &face))
                return false;            
        }
        
        FT_Set_Pixel_Sizes(face, 0, 1);
        
        glyph = face->glyph;      

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        std::string error = "";
        program = glCreateProgram();
        vertex = _glCreateShader(program, GL_VERTEX_SHADER, OpenGLTextVertexShader.c_str(), error);
        fragment = _glCreateShader(program, GL_FRAGMENT_SHADER, OpenGLTextFragmentShader.c_str(), error);
        _glLinkProgram( nullptr, program, error );
        glUseProgram( program );
        
        if (!error.empty()) {
            return false;
        }

        auto fontCoords = glGetAttribLocation(program, "fontCoords");
        glEnableVertexAttribArray(fontCoords);
        glVertexAttribPointer(fontCoords, 4, GL_FLOAT, GL_FALSE, 0, 0);
        
        return initialized = true;
    }
    
    auto showText(unsigned screenWidth, unsigned screenHeight, float xAdjust, float yAdjust, int align) -> void {
        if (disable || !textTex || !initialized) {
            return;
        }
	
		glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textTex);
		
        glUseProgram(program);
		glEnable(GL_BLEND);
		glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        float screenx = 2.0f / screenWidth, screeny = 2.0f / screenHeight;
        float textx = (float)totalWidth * screenx;
        float texty = (float)totalHeight * screeny;
        
        float x = -1.0;
        float y = 1.0;        
        
        if (align & ALIGN_CENTER) {
            x = 0.0 - textx / 2;
        } else if(align & ALIGN_RIGHT) {
            x = 1.0 - textx;
        }
        
        if (align & VALIGN_CENTER) {
            y = 0.0 + texty / 2;
        } else if (align & VALIGN_BOTTOM) {
            y = -1.0 + texty;
        }              
        
        x += xAdjust;
        y += yAdjust;        
        
        GLfloat box[4][4] = {
            {x, y, 0, 0},
            {x + textx, y, 1, 0},
            {x, y - texty, 0, 1},
            {x + textx, y - texty, 1, 1}
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_BLEND);
    }

    auto buildTexture(std::string text) -> void {
        disable = text.empty();
        
        if (disable || !initialized) 
            return;
            
        unsigned index = 0;
        unsigned code;
        totalWidth = 0;
        totalHeight = 0;
        unsigned upHeight = 0;
        unsigned downHeight = 0;
        
        while (code = utf8decode(text, index)) {
                                       
            if (FT_Load_Char(face, code, FT_LOAD_RENDER)) {
                continue;
            }
            
            totalWidth += (glyph->advance.x >> 6 );            
            
            upHeight = std::max(upHeight, (unsigned)(glyph->bitmap.rows ));
			
			if (glyph->bitmap_top < 0)
				continue;
            
            if (glyph->bitmap.rows > glyph->bitmap_top)
                downHeight = std::max(downHeight, glyph->bitmap.rows - glyph->bitmap_top);
        }
        
        totalHeight = upHeight + downHeight;
                
        if (textBuffer) {
            delete[] textBuffer;
            textBuffer = nullptr;
        }
                    
        if (textTex) {
            glDeleteTextures(1, &textTex);
            textTex = 0;            
        }
        
        unsigned _size = totalWidth * totalHeight;
        
        if (_size == 0) {
            disable = true;
            return;
        }            
            
        textBuffer = new unsigned char[ _size ];
        std::memset(textBuffer, 0, _size );

        index = 0;
        unsigned writePos = 0;
		unsigned curPos;
        
        while (code = utf8decode(text, index)) {
            if (FT_Load_Char(face, code, FT_LOAD_RENDER)) {
                continue;
            }
            
            for(unsigned i = 0; i < glyph->bitmap.rows; i++) {
                            
                for(unsigned j = 0; j < glyph->bitmap.width; j++) {
                                       
					if (glyph->bitmap_top < 0) {
						curPos = (totalHeight + glyph->bitmap_top + i) * totalWidth + writePos + j + glyph->bitmap_left;	
						
					} else {						
						curPos = (upHeight - glyph->bitmap_top + i) * totalWidth + writePos + j + glyph->bitmap_left;
					}
					
					if (curPos >= _size)
						break;
                                                           
                    *( textBuffer + curPos ) = glyph->bitmap.buffer[i * glyph->bitmap.width + j ];                                        
                }
            }
            writePos += (glyph->advance.x >> 6);
        }

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &textTex);
        glBindTexture(GL_TEXTURE_2D, textTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, totalWidth, totalHeight, 0, GL_RED, GL_UNSIGNED_BYTE, textBuffer);
    }
            
};
