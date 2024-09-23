
#include "../freetype.h"

namespace DRIVER {

    struct GlScreenText : Freetype {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint program = 0;
        GLuint textTex = 0;

        ~GlScreenText() {
            term();
        }

        auto init() -> bool {
            std::string error = "";
            GLuint vertex = 0;
            GLuint fragment = 0;
            GLint fontCoords;

            term();

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            vertex = GLUtility::createShader(GL_VERTEX_SHADER, OpenGLTextVertexShader.c_str(), error);
            fragment = GLUtility::createShader(GL_FRAGMENT_SHADER, OpenGLTextFragmentShader.c_str(), error);

            if (!vertex || !fragment)
                goto End;

            program = GLUtility::createProgram(vertex, fragment, error, true);

            if (!program)
                goto End;

            glUseProgram( program );

            fontCoords = glGetAttribLocation(program, "fontCoords");
            glEnableVertexAttribArray(fontCoords);
            glVertexAttribPointer(fontCoords, 4, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            ftInitialized = true;

            End:
            GLUtility::deleteShader(vertex);
            GLUtility::deleteShader(fragment);

            return ftInitialized;
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

            if (textTex) {
                glDeleteTextures(1, &textTex);
                textTex = 0;
            }
            ftInitialized = false;
        }

        auto ftSetCoordsPosition() -> void {
            glUseProgram(program);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof ftPosCoords, ftPosCoords, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        auto showText(Viewport& viewport) -> void {
            if (ftUpdated)
                ftProcessUpdates(viewport);

            if (!ftTextBuffer)
                return;

            if (ftTs) // animations
                if (ftHandleAnimation(viewport))
                    return;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textTex);

            glUseProgram(program);
            glEnable(GL_BLEND);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDisable(GL_BLEND);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        auto ftBuildTexture(std::string text, bool keepOldSize = false) -> void {
            if (!ftBuildText(text, keepOldSize))
                return;

            if (!textTex || !keepOldSize) {
                if (textTex)
                    glDeleteTextures(1, &textTex);

                glActiveTexture(GL_TEXTURE0);
                glGenTextures(1, &textTex);
                glBindTexture(GL_TEXTURE_2D, textTex);
                GLUtility::glParameters(GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            } else {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textTex);
            }

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ftTotalWidth, ftTotalHeight, 0, GL_RED, GL_UNSIGNED_BYTE, ftTextBuffer);
        }

        auto ftSetColor(FtColNorm& _colNorm, FtColNorm& _colBgNorm) -> void {
            if (program) {
                glUseProgram(program);
                glUniform4f(glGetUniformLocation(program, "color"), _colNorm.r, _colNorm.g, _colNorm.b, _colNorm.a);
                glUniform4f(glGetUniformLocation(program, "bgColor"), _colBgNorm.r, _colBgNorm.g, _colBgNorm.b, _colBgNorm.a);
            }
        }
    };

}
