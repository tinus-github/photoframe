//
//  gl-texture.h
//  Photoframe
//
//  Created by Martijn Vernooij on 13/05/16.
//
//

#ifndef gl_texture_h
#define gl_texture_h

#include <stdio.h>
#include "gl-object.h"

typedef struct gl_texture gl_texture;

typedef struct gl_texture_funcs {
	gl_object_funcs p;
	GLuint (*load_image) (gl_texture *obj, char *rgba_data, unsigned int width, unsigned int height);
	void (*gl_object_free) (gl_object *obj);
} gl_texture_funcs;

typedef struct gl_texture_data {
	gl_object_data p;
	GLuint textureId;
	int texture_loaded;
};

struct gl_texture {
	gl_texture_funcs *f;
	gl_texture_data data;
};

gl_texture *gl_texture_init(gl_texture *obj);
gl_texture *gl_texture_new();

#endif /* gl_texture_h */
