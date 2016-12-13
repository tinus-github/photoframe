//
//  gl-shape.c
//  Photoframe
//
//  Created by Martijn Vernooij on 16/05/16.
//
//

#include "gl-shape.h"
#include "gl-display.h"
#include <string.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

#include "../lib/linmath/linmath.h"

static void gl_shape_draw(gl_shape *obj);
static void gl_shape_set_projection(gl_shape *obj, mat4x4 new_projection);
static void gl_shape_set_computed_projection_dirty(gl_shape *obj);

static struct gl_shape_funcs gl_shape_funcs_global = {
	.draw = &gl_shape_draw,
	.set_projection = &gl_shape_set_projection,
	.set_computed_projection_dirty = &gl_shape_set_computed_projection_dirty
};

static void gl_shape_draw(gl_shape *obj)
{
	printf("%s\n", "gl_shape_draw is an abstract function");
	abort();
}

static void gl_shape_set_projection(gl_shape *obj, mat4x4 new_projection)
{
	mat4x4_dup(obj->data.projection, new_projection);
	obj->f->set_computed_projection_dirty(obj);
}

// TODO: export this?
static void gl_shape_clear_projection(gl_shape *obj)
{
	mat4x4 projection;
	
	mat4x4_identity(projection);
	obj->f->set_projection(obj, projection);
}

static void gl_shape_set_computed_projection_dirty(gl_shape *obj)
{
	obj->data.computed_projection_dirty = TRUE;
}

void gl_shape_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_shape_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

gl_shape *gl_shape_init(gl_shape *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_shape_funcs_global;
	
	gl_shape_clear_projection(obj);
	
	return obj;
}

gl_shape *gl_shape_new()
{
	gl_shape *ret = malloc(sizeof(gl_shape));
	
	return gl_shape_init(ret);
}
