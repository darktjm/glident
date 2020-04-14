/*
 * A libGL.so.1 "overlay" for wine.  This corrects an issue where Mesa rejects
 * numerous Stars in Shadow shaders.  Works with wine-2.13, at least.
 * To build:
 *   - Create a dummy libGL to link against.  This is to override the
 *     soname of the real libGL; otherwise a recursive dependency is
 *     created.  *DO NOT INSTALL THIS ANYWHERE*
 *     gcc -x c -shared -o libGL.so /dev/null
 *   - Compile this against the dummy library:
 *     gcc -fPIC -shared -o glident.{so,c} -ldl -L. -lGL
 * If you're using a 32-bit binary rather than 64-bit:
 *     gcc -x c -shared -o libGL.so -m32 /dev/null
 *     gcc -fPIC -m32 -shared -o glident.{so,c} -ldl -L. -lGL
 *
 * To use:
 *   - Move glident.so to libGL.so.1 in some directory of its own
 *     e.g. I put it in a dir called glident in the game dir:
 *     mkdir glident
 *     mv ~/src/glident.so glident/libGL.so.1
 *   - If your system doesn't have a libGL.so, just create a soft link to
 *     the real libGL.so.1 to libGL.so either in the system library path
 *     or in the same subdirectory you're using in the above step
 *   - set LD_LIBRARY_PATH to that directory when running wine:
 *     LD_LIBRARY_PATH=$PWD/glident wine <game>.exe
 */

/* for RTLD_NEXT and glibc's memmem */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <string.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define _GLX_PUBLIC

/*
 * allow env override of vendor/renderer/version string[s]
 */
const GLubyte *GLAPIENTRY glGetString( GLenum name )
{
    static void *next = NULL;
    /* I won't bother saving the env, since this function will probably
     * not be called more than once per type */
    static const char *str;
    switch(name) {
#define dostr(V) \
  case GL_##V: \
    str = getenv("GL_FAKE_" #V); \
    if(str) { \
	if(!next) \
	    next = dlsym(RTLD_NEXT, "glGetString"); \
	fprintf(stderr, "Overriding GL " #V " (%s) with '%s'\n", \
		((const GLubyte *GLAPIENTRY (*)(GLenum))next)(name), str); \
	return (const GLubyte *)str; \
    } \
    break
	dostr(VENDOR);
	dostr(RENDERER);
	dostr(VERSION);
    }
    if(!next)
	next = dlsym(RTLD_NEXT, "glGetString");
    return ((const GLubyte *GLAPIENTRY (*)(GLenum))next)(name);
}

/* This is how wine gets the address of functions, rather than dlsym() */
_GLX_PUBLIC void (*glXGetProcAddressARB(const GLubyte * procName)) (void)
{
    static void *next = NULL;
    /* Stars in Shadow seems to use glShaderSourceARB exclusively */
    if (!strcmp((const char *)procName, "glGetString"))
	return (void *)glGetString;
    if(!next) {
	next = dlsym(RTLD_NEXT, "glXGetProcAddressARB");
	if(!next)
	    exit(1);
	else
	    fputs("glident-hack loaded\n", stderr);
    }
    return ((_GLX_PUBLIC void (*(*)(const GLubyte *))(void))next)(procName);
}
