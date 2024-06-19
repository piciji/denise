
#include <thread>
#include "../freetype.h"

namespace DRIVER {

struct OpenGLText {

    struct {
        std::string message = "";
        bool messageUpdated = false;
        bool critical = false;
        bool criticalUpdated = false;
    } current;

    Freetype ft;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint program = 0;

    GLuint textTex = 0;
    std::mutex updateMutex;

    bool initialized = false;

    enum {
        ALIGN_LEFT/*Top*/ = 0,  ALIGN_CENTER = 1, ALIGN_RIGHT = 2, VALIGN_BOTTOM = 4, VALIGN_CENTER = 16
    };


    OpenGLText() {
    }

    ~OpenGLText() {
        term();
    }

    auto setFontSize(int value) -> void {
        ft.setFontSize(value);
    }

    auto setColor(float r, float g, float b, float a) -> void {
        if (program) {
            glUseProgram(program);
            glUniform4f(glGetUniformLocation(program, "color"), r, g, b, a);
        }
    }

    auto term() -> void {
        ft.term();

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

        initialized = false;
    }

    auto init() -> bool {
        std::string error = "";
        GLuint vertex = 0;
        GLuint fragment = 0;
        GLint fontCoords;

        term();
        if (!ft.init())
            return false;

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
        initialized = true;

        End:
        GLUtility::deleteShader(vertex);
        GLUtility::deleteShader(fragment);

        return initialized;
    }

    auto showText(unsigned screenWidth, unsigned screenHeight, float xAdjust, float yAdjust, int align) -> void {
        if (!ft.hasText() || !textTex || !initialized) {
            return;
        }

		glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textTex);

        glUseProgram(program);
		glEnable(GL_BLEND);
		glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        float screenx = 2.0f / screenWidth, screeny = 2.0f / screenHeight;
        float textx = (float)ft.totalWidth * screenx;
        float texty = (float)ft.totalHeight * screeny;

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

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    auto buildTexture(std::string& text) -> void {
        if (!ft.buildTexture(text))
            return;

        if (textTex)
            glDeleteTextures(1, &textTex);

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &textTex);
        glBindTexture(GL_TEXTURE_2D, textTex);
        GLUtility::glParameters(GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ft.totalWidth, ft.totalHeight, 0, GL_RED, GL_UNSIGNED_BYTE, ft.textBuffer);
    }

    auto updateMessage() -> void {
        updateMutex.lock();
        if (current.messageUpdated) {
            current.messageUpdated = false;
            std::string message = current.message;

            updateMutex.unlock();
            buildTexture( message );
            updateMutex.lock();
        }

        if (current.criticalUpdated) {
            current.criticalUpdated = false;
            bool critical = current.critical;
            updateMutex.unlock();

            if (ft.hasText()) {
                if (critical)
                    setColor(0.7f, 0.0f, 0.0f, 1.0f);
                else
                    setColor(1.0f, 1.0f, 1.0f, 0.8f);
            }
        } else
            updateMutex.unlock();
    }

    auto updateMessage(std::string message, bool critical, bool instant = true) -> void {

        if (!instant)
            updateMutex.lock();

        if (!initialized)
            return;

        if (current.message != message) {
            current.message = message;
            if (instant)
                buildTexture(message);
            else
                current.messageUpdated = true;
        }

        if (current.critical != critical) {
            current.critical = critical;

            if (instant) {
                if (ft.hasText()) {
                    if (critical)
                        setColor(0.7f, 0.0f, 0.0f, 1.0f);
                    else
                        setColor(1.0f, 1.0f, 1.0f, 0.8f);
                }
            } else
                current.criticalUpdated = true;
        }

        if (!instant)
            updateMutex.unlock();

    }
};

}
