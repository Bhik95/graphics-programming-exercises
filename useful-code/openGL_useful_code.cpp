//
// Created by frass on 06/10/2020.
// Get more logs out of OpenGL
//

#pragma once

#include <assert.h>
#include "glad/glad.h"
#include <stdio.h>

// Clears all errors from the OpenGL error queue, executes contained code...
#define GLCall(x) \
    GLClearErrors(); \
    (x);          \
    assert(GLLogCall((#x), __FILE__, __LINE__))

// Clear all errors from OpenGL Error queue
void GLClearErrors();

// Logs opengl errors raised during execution of 'function', with file and line
bool GLLogCall(const char* function, const char* file, long line);

// Map of error code to message, use documentation per function to find actual ...
const char* GetErrorMessageFromCode(unsigned int code);

void GLClearErrors()
{
    while(glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, long line)
{
    bool noErrors = true;
    GLenum error = glGetError();
    while(error != GL_NO_ERROR)
    {
        const char* msg = GetErrorMessageFromCode(error);
        printf("[OpenGL Error 0x%X] %s | %s | Line %d\n\t%s\n", error, file, function, line, msg);
        noErrors = false;
        error = glGetError();
    }

    return noErrors;
}

const char* GetErrorMessageFromCode(unsigned int code)
{
    switch(code)
    {
        case 0x500:
            return "GL_INVALID_ENUM";
        case 0x501:
            return "GL_INVALID_VALUE";
        case 0x502:
            return "GL_INVALID_OPERATION";
        case 0x503:
            return "GL_STACK_OVERFLOW";
        case 0x504:
            return "GL_STACK_UNDERFLOW";
        case 0x505:
            return "GL_OUT_OF_MEMORY";
        case 0x506:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case 0x507:
            return "GL_CONTEXT_LOST";
        default:
            printf("Unknown error code %d\n", code);
            return "";
    }
}
