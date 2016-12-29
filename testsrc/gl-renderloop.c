//
//  gl-renderloop.c
//  Photoframe
//
//  Created by Martijn Vernooij on 27/12/2016.
//
//

#include "gl-renderloop.h"
#include "gl-renderloop-member.h"
#include <string.h>
#include <assert.h>

static void gl_renderloop_append_child(gl_renderloop *obj, gl_renderloop_phase phase, gl_renderloop_member *child);
static void gl_renderloop_remove_child(gl_renderloop *obj, gl_renderloop_member *child);
static void gl_renderloop_run(gl_renderloop *obj);

static struct gl_renderloop_funcs gl_renderloop_funcs_global = {
	.append_child = &gl_renderloop_append_child,
	.remove_child = &gl_renderloop_remove_child,
	.run = &gl_renderloop_run
};

void (*parent_free_f) (gl_object *obj);

gl_renderloop *global_renderloop = NULL;

void gl_renderloop_setup()
{
	if (global_renderloop) {
		return;
	}
	gl_object *parent = gl_object_new();
	memcpy(&gl_renderloop_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	gl_object_funcs *object_funcs = (gl_object_funcs *)&gl_renderloop_funcs_global;
	parent_free_f = object_funcs->free;
	object_funcs->free = &gl_renderloop_free;
	
	global_renderloop = gl_renderloop_new();
}

gl_renderloop *gl_renderloop_init(gl_renderloop *obj)
{
	gl_object_init((gl_object*)obj);
	
	obj->f = &gl_renderloop_funcs_global;
	
	unsigned int counter;
	gl_renderloop_member *member;
	for (counter = 0; counter < GL_RENDERLOOP_PHASES; counter++) {
		member = gl_renderloop_member_new();
		obj->data.phaseHead[counter] = member;
		member->data.siblingL = member;
		member->data.siblingR = member;
	}
	
	return obj;
}

void gl_renderloop_free(gl_object * obj_obj)
{
	gl_renderloop *obj = (gl_renderloop *)obj_obj;
	
	unsigned int counter;
	gl_renderloop_member *member;
	gl_object *member_obj;
	for (counter = 0; counter < GL_RENDERLOOP_PHASES; counter++) {
		member = obj->data.phaseHead[counter];
		member_obj = (gl_object *)member;
		member_obj->f->unref(member_obj);
		obj->data.phaseHead[counter] = NULL;
	}
}

gl_renderloop *gl_renderloop_new()
{
	gl_renderloop *ret = calloc(1, sizeof(gl_renderloop));
	
	return gl_renderloop_init(ret);
}

static void gl_renderloop_remove_child(gl_renderloop *obj, gl_renderloop_member *child)
{
	assert (child->data.owner == obj);

	gl_renderloop_member *siblingR = child->data.siblingR;
	gl_renderloop_member *siblingL = child->data.siblingL;
	
	siblingR->data.siblingL = child->data.siblingL;
	siblingL->data.siblingR = child->data.siblingR;
	
	child->data.owner = NULL;
	obj_child->f->unref(obj_child);
}

static void gl_renderloop_append_child(gl_renderloop *obj, gl_renderloop_phase phase, gl_renderloop_member *child)
{
	gl_object *obj_child = (gl_object *)child;
	obj_child->f->ref(obj_child); // Prevent the child from being deallocated as it is being removed from the parent

	if (child->data.owner) {
		gl_renderloop *owner = child->data.owner;
		owner->f->remove_child(owner, child);
	}
	
	child->data.owner = obj;
	child->data.renderloopPhase = phase;

	gl_renderloop_member *head = obj->data.phaseHead[phase];
	gl_renderloop_member *last_child = head->data.siblingL;
	child->data.siblingL = last_child;
	last_child->data.siblingR = child;
	child->data.siblingR = first_child;
	head->data.siblingL = child;

	obj_child->f->unref(obj_child);
}

static void gl_renderloop_run_phase(gl_renderloop *obj, gl_renderloop_phase phase)
{
	unsigned int done = 0;
	
	gl_object *current_child_object;
	
	if (!obj->data.phaseFirstChild[phase]) {
		return;
	}
	
	gl_renderloop_member *head = obj->data.phaseHead[phase];
	gl_renderloop_member *current_child = head->data.siblingR;
	gl_renderloop_member *next_child = current_child->data.siblingR;
	
	while (current_child != head) {
		assert (current_child->data.owner = obj);
		
		current_child->f->run_action(current_child);
		
		current_child = next_child;
		next_child = current_child->data.siblingR;
	}
}

static void gl_renderloop_run(gl_renderloop *obj)
{
	gl_renderloop_phase current_phase;
	for (current_phase = 0; current_phase < GL_RENDERLOOP_PHASES; current_phase++) {
		gl_renderloop_run_phase(obj, current_phase);
	}
}

gl_renderloop *gl_renderloop_get_global_renderloop()
{
	return global_renderloop;
}
