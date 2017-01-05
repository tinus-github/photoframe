//
//  gl-label.c
//  Photoframe
//
//  Created by Martijn Vernooij on 02/01/2017.
//
//

#include "gl-label.h"
#include <assert.h>
#include <string.h>


static void gl_label_free(gl_object *obj);
static void gl_label_render(gl_label *obj);

static struct gl_label_funcs gl_label_funcs_global = {
	.render = &gl_label_render
};

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_label_render(gl_label *obj)
{
	gl_label_renderer *renderer = obj->data.renderer;
	renderer->data.text = obj->data.text;
	
	renderer->f->layout(renderer);
	// TODO: check final width
	obj->data.tile = renderer->f->render(renderer,
					     0, 0,
					     obj->data.width,
					     obj->data.height);
	
}

void gl_label_setup()
{
	gl_shape *parent = gl_shape_new();
	gl_object *parent_obj = (gl_object *)parent;
	memcpy(&gl_label_funcs_global.p, parent->f, sizeof(gl_shape_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_label_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_label_free;
	
	parent_obj->f->free(parent_obj);
	
	gl_label_renderer_setup();
}

gl_label *gl_label_init(gl_label *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_label_funcs_global;
	
	return obj;
}

gl_label *gl_label_new()
{
	gl_label *ret = calloc(1, sizeof(gl_label));
	
	ret->data.renderer = gl_label_renderer_new();
	
	return gl_label_init(ret);
}

void gl_label_free(gl_object *obj_obj)
{
	gl_label *obj = (gl_label *)obj_obj;
	free (obj->data.text);
	
	gl_object *renderer_obj = (gl_object *)obj->data.renderer;
	renderer_obj->f->unref(renderer_obj);
	
	gl_object_free_org_global(obj_obj);
}
