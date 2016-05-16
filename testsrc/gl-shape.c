//
//  gl-shape.c
//  Photoframe
//
//  Created by Martijn Vernooij on 16/05/16.
//
//

#include "gl-shape.h"
#include <string.h>
#include <stdio.h>

static void gl_shape_draw(gl_shape *obj);

static struct gl_shape_funcs gl_shape_funcs_global = {
	.draw = &gl_shape_draw
};

static void gl_shape_draw(gl_shape *obj)
{
	printf("%s\n", "gl_shape_draw is an abstract function");
	abort();
}

void gl_shape_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_shape_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

