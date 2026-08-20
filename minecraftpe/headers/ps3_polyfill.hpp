#pragma once

#include <stdio.h>

#ifdef __cplusplus
#include <cmath>
extern "C" {
    #else
    #include <math.h>
    #endif

    // Esto evita que C++ "mutile" los nombres de las funciones de hilos
    #include <pthread.h>
    #include <sys/select.h>
    #include <stddef.h>

    #ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#define _GLIBCXX_MUTEX 1
#define _GLIBCXX_MUTEX_H 1
#define _GLIBCXX_UNIQUE_LOCK_H 1
#define _GLIBCXX_CONDITION_VARIABLE 1
#define _GLIBCXX_THREAD 1
namespace std {
    class mutex { public: mutex(){} ~mutex(){} void lock(){} void unlock(){} };
    struct defer_lock_t { }; const defer_lock_t defer_lock = {};
    template <class Mutex> class unique_lock { public: unique_lock(){} unique_lock(Mutex& m){} unique_lock(Mutex& m, defer_lock_t){} ~unique_lock(){} void lock(){} void unlock(){} };
    class condition_variable { public: condition_variable(){} ~condition_variable(){} void notify_one(){} void notify_all(){} template <class Lock> void wait(Lock& lock){} };
    template <class Mutex> class lock_guard { public: lock_guard(Mutex& m){} ~lock_guard(){} };
    class thread { public: thread(){} template<typename Callable> thread(Callable c){} ~thread(){} void join(){} };
    inline float exp2(float x) { return std::pow(2.0f, x); }
    inline double exp2(double x) { return std::pow(2.0, x); }
    inline float log2(float x) { return std::log(x) / 0.6931471805599453f; }
    inline double log2(double x) { return std::log(x) / 0.6931471805599453; }
    inline float round(float x) { return std::floor(x + 0.5f); }
    inline double round(double x) { return std::floor(x + 0.5); }
    inline float trunc(float x) { return x < 0.0f ? std::ceil(x) : std::floor(x); }
    inline double trunc(double x) { return x < 0.0 ? std::ceil(x) : std::floor(x); }
    inline float fma(float x, float y, float z) { return (x * y) + z; }
    inline double fma(double x, double y, double z) { return (x * y) + z; }
    inline float asinh(float x) { return std::log(x + std::sqrt(x * x + 1.0f)); }
    inline double asinh(double x) { return std::log(x + std::sqrt(x * x + 1.0)); }
    inline float acosh(float x) { return std::log(x + std::sqrt(x * x - 1.0f)); }
    inline double acosh(double x) { return std::log(x + std::sqrt(x * x - 1.0)); }
    inline float atanh(float x) { return 0.5f * std::log((1.0f + x) / (1.0f - x)); }
    inline double atanh(double x) { return 0.5 * std::log((1.0 + x) / (1.0 - x)); }
}
#endif

// ==========================================
// CONSTANTES DE OPENGL
// ==========================================
#define GL_EXTENSIONS 0x1F03
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
#define GL_TEXTURE_2D 0x0DE1
#define GL_BLEND 0x0BE2
#define GL_DEPTH_TEST 0x0B71
#define GL_CULL_FACE 0x0B44
#define GL_ALPHA_TEST 0x0BC0
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_DEPTH_BUFFER_BIT 0x0100
#define GL_PROJECTION 0x1701
#define GL_MODELVIEW 0x1700
#define GL_RGBA 0x1908
#define GL_RGB 0x1907
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_FLOAT 0x1406
#define GL_LINEAR 0x2601
#define GL_NEAREST 0x2600
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_QUAD_STRIP 0x0008
#define GL_POLYGON 0x0009
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_ALWAYS 0x0207
#define GL_LEQUAL 0x0203
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_START 0x0B63
#define GL_FOG_END 0x0B64
#define GL_FOG_MODE 0x0B65
#define GL_FOG_COLOR 0x0B66
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_COLOR_ARRAY 0x8076
#define GL_NORMAL_ARRAY 0x8075
#define GL_VERTEX_ARRAY 0x8074

#ifdef __cplusplus
extern "C" {
    #endif

    void glMatrixMode(unsigned int mode);
    void glLoadIdentity(void);
    void glPushMatrix(void);
    void glPopMatrix(void);
    void glMultMatrixf(const float *m);
    void glTranslatef(float x, float y, float z);
    void glScalef(float x, float y, float z);
    void glRotatef(float angle, float x, float y, float z);
    void gluPerspective(float fovy, float aspect, float zNear, float zFar);

     void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar);

    void glClearColor(float red, float green, float blue, float alpha);
    void glClear(unsigned int mask);

    void glEnable(unsigned int cap);
    void glDisable(unsigned int cap);
    void glBlendFunc(unsigned int sfactor, unsigned int dfactor);
    void glAlphaFunc(unsigned int func, float ref);
    void glStencilFunc(unsigned int func, int ref, unsigned int mask);
    void glEnableClientState(unsigned int array);
    void glDisableClientState(unsigned int array);

    void glVertexPointer(int size, unsigned int type, int stride, const void *pointer);
    void glTexCoordPointer(int size, unsigned int type, int stride, const void *pointer);
    void glColorPointer(int size, unsigned int type, int stride, const void *pointer);
    void glNormalPointer(unsigned int type, int stride, const void *pointer);
    void glDrawArrays(unsigned int mode, int first, int count);
    void glDrawElements(unsigned int mode, int count, unsigned int type, const void *indices);
    void glGetFloatv(unsigned int pname, float *params);
    void glViewport(int x, int y, int width, int height);
    void glScissor(int x, int y, int width, int height);

    void glCullFace(unsigned int mode);
    void glFrontFace(unsigned int mode);
    void glDepthRangef(float n, float f);
    void glDepthRange(double n, double f);
    void glDepthFunc(unsigned int func);

    void glGenTextures(int n, unsigned int *tex);
    void glBindTexture(unsigned int target, unsigned int texture);
    void glTexImage2D(unsigned int target, int level, int internalformat, int width, int height, int border, unsigned int format, unsigned int type, const void *pixels);

    void glColor4f(float red, float green, float blue, float alpha);
    void glDepthMask(unsigned char flag);
    void glColorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha); // <--- AQUI SE AÑADE GLCOLORMASK
    void glDeleteTextures(int n, const unsigned int *textures);
    void glTexSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void *pixels);

    extern void (*glDeleteBuffers)(int n, const unsigned int* buffers);
    extern void (*glGenBuffers)(int n, unsigned int *buffers);
    extern void (*glBindBuffer)(unsigned int target, unsigned int buffer);
    extern void (*glBufferData)(unsigned int target, ptrdiff_t size, const void *data, unsigned int usage);

    static inline void glColor3f(float red, float green, float blue) {
        glColor4f(red, green, blue, 1.0f);
    }


    static inline void glLightfv(unsigned int light, unsigned int pname, const float *params) {}
    static inline void glLightModelf(unsigned int pname, float param) {}
    static inline void glLightModelfv(unsigned int pname, const float *params) {}
    static inline void glTexParameteri(unsigned int target, unsigned int pname, int param) {}
    static inline void glTexParameterf(unsigned int target, unsigned int pname, float param) {}
    static inline void glFogf(unsigned int pname, float param) {}
    static inline void glFogfv(unsigned int pname, const float *params) {}
    static inline void glFogx(unsigned int pname, int param) {}
    static inline void glFogi(unsigned int pname, int param) {}
    static inline void glHint(unsigned int target, unsigned int mode) {}
    static inline void glLineWidth(float width) {}
    static inline void glNormal3f(float nx, float ny, float nz) {}
    static inline void glShadeModel(unsigned int mode) {}
    static inline void glPolygonOffset(float factor, float units) {}
    static inline void glReadPixels(int x, int y, int width, int height, unsigned int format, unsigned int type, void *pixels) {}
    static inline unsigned int glGetError(void) { return 0; }
    static inline void glGetIntegerv(unsigned int pname, int *params) {}
    static inline const unsigned char* glGetString(unsigned int name) { return (const unsigned char*)"PS3 RSX Wrapper"; }
    static inline void glActiveTexture(unsigned int texture) {}
    static inline void glClientActiveTexture(unsigned int texture) {}
    static inline void glBegin(unsigned int mode) {}
    static inline void glEnd(void) {}
    static inline void glVertex2f(float x, float y) {}
    static inline void glVertex3f(float x, float y, float z) {}
    static inline void glTexCoord2f(float s, float t) {}
    static inline void glStencilMask(unsigned int mask) {}
    static inline void glClearStencil(int s) {}
    static inline int gethostname(char *name, unsigned int len) { return -1; }

    #ifdef __cplusplus
}
#endif
