//
//  gl-workqueue.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/01/2017.
//
//

#include <stdlib.h>
#include <string.h>
#include "infrastructure/gl-workqueue.h"
#include "infrastructure/gl-workqueue-job.h"
#include "gl-renderloop.h"
#include "gl-renderloop-member.h"

static void gl_workqueue_append_job(gl_workqueue *obj, gl_workqueue_job *job);

static struct gl_workqueue_funcs gl_workqueue_funcs_global = {
	.append_job = &gl_workqueue_append_job
};

// Must lock queueMutex in advance!
static void gl_workqueue_append_job_to_queue_nl(gl_workqueue *obj, gl_workqueue_job *job, gl_workqueue_job *head)
{
	gl_workqueue_job *last_job = head->data.siblingL;
	
	head->data.siblingL = last_job;
	last_job->data.siblingR = job;
	job->data.siblingR = head;
	head->data.siblingL = job;
}

static gl_workqueue_job *gl_workqueue_remove_job_nl(gl_workqueue *obj, gl_workqueue_job *job)
{
	gl_workqueue_job *siblingR = job->data.siblingR;
	gl_workqueue_job *siblingL = job->data.siblingL;
	
	siblingR->data.siblingL = job->data.siblingL;
	siblingL->data.siblingR = job->data.siblingR;
	
	job->data.siblingL = job;
	job->data.siblingR = job;
	
	return job;
}

static void gl_workqueue_append_job(gl_workqueue *obj, gl_workqueue_job *job)
{
	pthread_mutex_lock(&obj->data.queueMutex);
	
	gl_workqueue_append_job_to_queue_nl(obj, job, obj->data.queuedJobs);
	
	pthread_cond_signal(&obj->data.workAvailable);
	
	pthread_mutex_unlock(&obj->data.queueMutex);
}

static gl_workqueue_job *gl_workqueue_pop_first_job_nl(gl_workqueue *obj, gl_workqueue_job *head)
{
	gl_workqueue_job *job = head->data.siblingR;
	
	if (job == head) {
		return NULL;
	}
	gl_workqueue_remove_job_nl(obj, job);
	
	return job;
}

static void gl_workqueue_runloop(gl_workqueue *obj)
{
	gl_workqueue_job *job;
	
	pthread_mutex_lock(&obj->data.queueMutex);
	while (1) {
		job = gl_workqueue_pop_first_job_nl(obj, obj->data.queuedJobs);
		if (!job) {
			pthread_cond_wait(&obj->data.workAvailable, &obj->data.queueMutex);
		} else {
			pthread_mutex_unlock(&obj->data.queueMutex);
			
			job->f->run(job);
			
			pthread_mutex_lock(&obj->data.queueMutex);
			gl_workqueue_append_job_to_queue_nl(obj, job, obj->data.doneJobs);
		}
	}
}

static void gl_workqueue_complete_jobs(gl_workqueue *obj, gl_renderloop_member *renderloop_member, void *action_data)
{
	gl_workqueue_job *job;
	pthread_mutex_lock(&obj->data.queueMutex);
	while (job = gl_workqueue_pop_first_job_nl(obj, obj->data.doneJobs)) {
		pthread_mutex_unlock(&obj->data.queueMutex);
		
		gl_notice *notice = job->f->doneNotice;
		notice->f->fire(notice);
		
		((gl_object *)job)->f->unref((gl_object *)job);
	}
	pthread_mutex_unlock(&obj->data.queueMutex);
}

void gl_workqueue_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_workqueue_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

gl_workqueue *gl_workqueue_init(gl_workqueue *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_workqueue_funcs_global;
	
	
	pthread_mutex_init(&obj->data.queueMutex, NULL);
	pthread_cond_init(&obj->data.workAvailable, NULL);
	
	gl_workqueue_job *head = gl_workqueue_job_new();
	head->data.siblingL = head;
	head->data.siblingR = head;
	obj->data.queuedJobs = head;
	
	head = gl_workqueue_job_new();
	head->data.siblingL = head;
	head->data.siblingR = head;
	obj->data.doneJobs = head;
	
	return obj;
}

gl_workqueue *gl_workqueue_new()
{
	gl_workqueue *ret = calloc(1, sizeof(gl_workqueue));
	
	return gl_workqueue_init(ret);
}
