#ifndef UNIGL_H
#define UNIGL_H

// ==========================================
// RUTA PARA PS3 (Sin OpenGL Nativo)
// ==========================================
#if defined(__PS3__)

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;
typedef long int GLsizeiptr;
typedef long int GLintptr;

#define GL_FALSE 0
#define GL_TRUE 1

#ifdef __cplusplus
extern "C" {
    #endif

    void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat znear, GLfloat zfar);
    int glhUnProjectf(float winx, float winy, float winz, float* modelview, float* projection, int* viewport, float* objectCoordinate);

    void initGlFuncs();
    extern void (*glDeleteBuffers)(GLsizei n, const GLuint* buffers);
    extern void (*glGenBuffers)(GLsizei n, GLuint* buffers);
    extern void (*glBindBuffer)(GLenum target, GLuint buffer);
    extern void (*glBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);

    #ifdef __cplusplus
}
#endif

// ==========================================
// RUTA ORIGINAL (Android / PC)
// ==========================================
#else

#ifdef USEGLES
#include <GLES/gl.h>
#include <GLES/egl.h>
#include <GLES/glext.h>

#ifdef __cplusplus
extern "C" {
    #endif
    void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat znear, GLfloat zfar);
    #ifdef __cplusplus
}
#endif

#else // No USEGLES

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#ifdef __cplusplus
extern "C" {
    #endif
    void initGlFuncs();
    extern void (*glDeleteBuffers)(GLsizei n, const GLuint* buffers);
    extern void (*glGenBuffers)(GLsizei n, GLuint* buffers);
    extern void (*glBindBuffer)(GLenum target, GLuint buffer);
    extern void (*glBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
    #ifdef __cplusplus
}
#endif

#endif // USEGLES

#ifdef __cplusplus
extern "C" {
    #endif
    int glhUnProjectf(float winx, float winy, float winz, float* modelview, float* projection, int* viewport, float* objectCoordinate);
    #ifdef __cplusplus
}
#endif

#endif // __PS3__
#endif // UNIGL_H
