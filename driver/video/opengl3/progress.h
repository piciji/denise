
#include <thread>
#include "../freetype.h"

namespace DRIVER {

    struct OpenGLProgress {

        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint program = 0;
        unsigned width;
        unsigned height;
        GLint locRotation;

        int rotation = 0;

        GLuint tex = 0;
        bool initialized = false;

        ~OpenGLProgress() {
            term();
        }

        auto term() -> void {
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

            if (tex) {
                glDeleteTextures(1, &tex);
                tex = 0;
            }

            initialized = false;
        }

        auto init() -> bool {
            std::string error = "";
            GLuint vertex = 0;
            GLuint fragment = 0;
            GLint texCoords;
            GLint locSource;

            term();

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            vertex = GLUtility::createShader(GL_VERTEX_SHADER, OpenGLProgressVertexShader.c_str(), error);
            fragment = GLUtility::createShader(GL_FRAGMENT_SHADER, OpenGLProgressFragmentShader.c_str(), error);

            if (!vertex || !fragment)
                goto End;

            program = GLUtility::createProgram(vertex, fragment, error, true);

            if (!program)
                goto End;

            glUseProgram( program );

            texCoords = glGetAttribLocation(program, "texCoords");
            glEnableVertexAttribArray(texCoords);
            glVertexAttribPointer(texCoords, 4, GL_FLOAT, GL_FALSE, 0, 0);

            locSource = glGetUniformLocation(program, "source");            
            locRotation = glGetUniformLocation(program, "degree");

            glUniform1i(locSource, 0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            initialized = true;

            End:
            GLUtility::deleteShader(vertex);
            GLUtility::deleteShader(fragment);

            return initialized;
        }

        auto setIcon(uint8_t* data, unsigned _width, unsigned _height) -> void {
            if (tex)
                glDeleteTextures(1, &tex);

            glGenTextures(1, &tex );
            glBindTexture(GL_TEXTURE_2D, tex);
            GLUtility::glParameters(GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (uint32_t*)data);
            glBindTexture(GL_TEXTURE_2D, 0);

            this->width = _width;
            this->height = _height;
        }

        auto show(Viewport& _viewport, unsigned xAdjust, unsigned yAdjust) -> void {
            if (!initialized) {
                return;
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);

            glUseProgram(program);
            glUniform1i(locRotation, rotation);

            glEnable(GL_BLEND);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            float screenx = 2.0f / _viewport.width, screeny = 2.0f / _viewport.height;
            float x = -1.0 + (float)(_viewport.width - width - xAdjust) * screenx;
            float y = 1.0 -  (float)yAdjust * screeny;

            float w = width * screenx;
            float h = height * screeny;

            float box[4][4] = {
                    {x,     y,     0, 0},
                    {x + w, y,     1, 0},
                    {x,     y - h, 0, 1},
                    {x + w, y - h, 1, 1}
            };

            glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDisable(GL_BLEND);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            if (++rotation == 360)
                rotation = 0;
        }

    };

}
