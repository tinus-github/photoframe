
/*
 * code stolen from openGL-RPi-tutorial-master/encode_OGL/
 * and from OpenGL® ES 2.0 Programming Guide
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <bcm_host.h>

#include "loadimage.h"
#include "gl-texture.h"
#include "gl-tile.h"
#include "gl-container-2d.h"
#include "gl-stage.h"
#include "gl-tiled-image.h"
#include "gl-renderloop.h"
#include "egl-driver.h"
#include "gl-value-animation.h"
#include "gl-value-animation-easing.h"
#include "labels/gl-label-scroller.h"
#include "infrastructure/gl-workqueue.h"
#include "infrastructure/gl-workqueue-job.h"
#include "infrastructure/gl-notice-subscription.h"
#include "gl-image.h"

#include "../lib/linmath/linmath.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

struct image_display_data {
	gl_container_2d *container_2d;
};

void fade_in_image(void *target, gl_notice_subscription *sub, void *extra_data);

void image_set_alpha(void *target, void *extra_data, GLfloat value)
{
	gl_shape *image_shape = (gl_shape *)target;
	
	image_shape->data.alpha = value;
	image_shape->f->set_computed_alpha_dirty(image_shape);
}

void fade_in_image(void *target, gl_notice_subscription *sub, void *extra_data)
{
	gl_image *img = (gl_image *)target;
	
	gl_container_2d *container_2d = (gl_container_2d *)extra_data;
	
	((gl_container *)gl_container_2d)->f->append_child(
							   (gl_container *)gl_container_2d, (gl_shape *)img);
	
	gl_value_animation_easing *animation_e = gl_value_animation_easing_new();
	animation_e->data.easingType = gl_value_animation_ease_linear;
	
	gl_value_animation *animation = (gl_value_animation *)animation_e;
	animation->data.startValue = 0.0;
	animation->data.endValue = 1.0;
	animation->data.duration = 0.4;
	animation->data.target = target;
	animation->data.action = image_set_alpha;
	
	animation->f->start(animation);
	image_set_alpha(target, NULL, 0.0);
}

int main(int argc, char *argv[])
{
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	
	//image = esLoadTGA("jan.tga", &width, &height);
	gettimeofday ( &t1 , &tz );
	
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return -1;
	}
	
	egl_driver_setup();
	egl_driver_init();

	gl_container_2d *image_container_2d = gl_container_2d_new();
	
	gl_image *img = gl_image_new();
	
	gl_notice_subscription *sub = gl_notice_subscription_new();
	sub->data.target = img;
	sub->data.action = &fade_in_image;
	sub->data.action_data = sub;
	
	img->data.readyNotice->f->subscribe(img->data.readyNotice, sub);
	img->f->load_file(img, argv[1]);
	
#if 0
	if (image == NULL) {
		fprintf(stderr, "No such image\n");
		exit(1);
	}
	fprintf(stderr, "Image is %d x %d\n", width, height);
	gettimeofday(&t2, &tz);
	deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
	
	printf("Image loaded in %1.4f seconds\n", deltatime);
#endif
	
	gl_container_2d *main_container_2d = gl_container_2d_new();
	gl_container *main_container_2d_container = (gl_container *)main_container_2d;
	gl_shape *main_container_2d_shape = (gl_shape *)main_container_2d;

	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)image_container_2d);

	gl_label_scroller *scroller = gl_label_scroller_new();
	gl_shape *scroller_shape = (gl_shape *)scroller;
	scroller_shape->data.objectHeight = 160;
	scroller_shape->data.objectWidth = 1920;
	scroller->data.text = "AVAVAVABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	scroller->f->start(scroller);
	main_container_2d_container->f->append_child(main_container_2d_container, scroller_shape);

	main_container_2d_shape->data.objectX = 0.0;
	main_container_2d->data.scaleH = 1.0;
	main_container_2d->data.scaleV = 1.0;
	
	gl_stage *global_stage = gl_stage_get_global_stage();
	global_stage->f->set_shape(global_stage, (gl_shape *)main_container_2d);
	
	gl_renderloop_loop();
	
	return 0; // not reached
}
