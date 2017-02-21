//
//  loadimage-jpg.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/02/2017.
//
//

#define TRUE 1
#define FALSE 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <stdint.h>
#include <setjmp.h>

#include <jpeglib.h>

#include "images/loadimage-jpg.h"
#include "images/gl-bitmap-scaler.h"
#include "images/loadexif.h"

struct decode_error_manager {
	struct jpeg_error_mgr org;
	jmp_buf setjmp_buffer;
};

typedef struct decode_error_manager * decode_error_manager;

/* Error handling/ignoring */

static void handle_decode_error(j_common_ptr info)
{
	decode_error_manager jerr = (decode_error_manager)info->err;
	(*info->err->output_message) (info);
	longjmp (jerr->setjmp_buffer, 1);
}

/* optimized builtin scaling */
static void setup_dct_scale(struct jpeg_decompress_struct *cinfo, float scalefactor)
{
	/* The library provides for accelerated scaling at fixed ratios of 1/4 and 1/2.
	 * This keeps some margin to prevent scaling artifacts
	 */
	
	if (scalefactor < 0.23f) {
		cinfo->scale_num = 1; cinfo->scale_denom = 4;
		return;
	}
	if (scalefactor < 0.46f) {
		cinfo->scale_num = 1; cinfo->scale_denom = 2;
		return;
	}
	return;
}

/* Returns true if this orientation flips width and height */
static boolean orientation_flips(unsigned int orientation)
{
	switch (orientation) {
		case 1:
		case 2:
		case 3:
		case 4:
			return FALSE;
		case 5:
		case 6:
		case 7:
		case 8:
			return TRUE;
		default:
			return FALSE;
	}
}

unsigned char *loadJPEG ( char *fileName, int wantedwidth, int wantedheight,
		  int *width, int *height, unsigned int *orientation )
{
	struct jpeg_decompress_struct cinfo;
	
	struct decode_error_manager jerr;
	
	struct loadimage_jpeg_client_data client_data;
	
	FILE *f;
	
	unsigned char *buffer = NULL;
	
	unsigned char *scanbuf = NULL;
	unsigned char *scanbufcurrentline;
	unsigned int scanbufheight;
	unsigned int lines_in_scanbuf = 0;
	unsigned int lines_in_buf = 0;
	
	JSAMPROW *row_pointers = NULL;
	
	unsigned int counter;
	unsigned int inputoffset;
	unsigned int outputoffset;
	unsigned char *outputcurrentline;
	
	float scalefactor, scalefactortmp;
	
	gl_bitmap_scaler *scaler = NULL;
	
	cinfo.err = jpeg_std_error(&jerr.org);
	jerr.org.error_exit = handle_decode_error;
	if (setjmp(jerr.setjmp_buffer)) {
		/* Something went wrong, abort! */
		jpeg_destroy_decompress(&cinfo);
		fclose(f);
		free(buffer);
		free(scanbuf);
		free(row_pointers);
		if (scaler) {
			((gl_object *)scaler)->f->unref((gl_object *)scaler);
		}
		return NULL;
	}
	
	f = fopen(fileName, "rb");
	if (!f) {
		return NULL;
	}
	
	jpeg_create_decompress(&cinfo);
	cinfo.client_data = &client_data;
	
	jpeg_stdio_src(&cinfo, f);
	
	loadexif_setup_overlay(&cinfo);
	
	jpeg_read_header(&cinfo, TRUE);
	
	loadexif_parse(&cinfo);
	*orientation = loadexif_get_orientation(&cinfo);
	
	if (orientation_flips(*orientation)) {
		int tmpheight = wantedheight;
		wantedheight = wantedwidth;
		wantedwidth = tmpheight;
	}
	
	cinfo.out_color_space = JCS_RGB;
	
	scalefactor = (float)wantedwidth / cinfo.image_width;
	scalefactortmp = (float)wantedheight / cinfo.image_height;
	
	if (scalefactortmp < scalefactor) {
		scalefactor = scalefactortmp;
	}
	setup_dct_scale(&cinfo, scalefactor);
	
	jpeg_start_decompress(&cinfo);
	
	scalefactor = (float)wantedwidth / cinfo.output_width;
	scalefactortmp = (float)wantedheight / cinfo.output_height;
	
	if (scalefactortmp < scalefactor) {
		scalefactor = scalefactortmp;
	}
	
	scaler = gl_bitmap_scaler_new();
	
	scaler->data.inputWidth = cinfo.image_width;
	scaler->data.inputHeight = cinfo.image_height;
	scaler->data.outputWidth = cinfo.output_width * scalefactor;
	scaler->data.outputHeight = cinfo.output_height * scalefactor;
	scaler->data.inputType = gl_bitmap_scaler_input_type_rgb;
	
	scaler->data.horizontalType = gl_bitmap_scaler_type_bresenham;
	scaler->data.verticalType = gl_bitmap_scaler_type_bresenham;
	
	scaler->f->start(scaler);
	
	*width = cinfo.output_width * scalefactor;
	*height = cinfo.output_height * scalefactor;
	
	buffer = malloc(width[0] * height[0] * 4);
	
	scanbufheight = cinfo.rec_outbuf_height;
	scanbuf = malloc(cinfo.output_width * 3 * scanbufheight);
	row_pointers = malloc(scanbufheight * sizeof(JSAMPROW));
	for (counter = 0; counter < scanbufheight; counter++) {
		row_pointers[counter] = scanbuf + 3 * (counter * cinfo.output_width);
	}
	
	while (lines_in_buf < cinfo.output_height) {
		if (!lines_in_scanbuf) {
			lines_in_scanbuf = jpeg_read_scanlines(
							       &cinfo, row_pointers, scanbufheight);
			scanbufcurrentline = scanbuf;
		}
		if (scalefactor != 1.0f) {
			scaler->f->process_line(scaler, buffer, scanbufcurrentline);
		} else {
			inputoffset = outputoffset = 0;
			outputcurrentline = buffer + 4  * (lines_in_buf * cinfo.output_width);
			for (counter = 0; counter < cinfo.output_width; counter++) {
				outputcurrentline[outputoffset++] = scanbufcurrentline[inputoffset++];
				outputcurrentline[outputoffset++] = scanbufcurrentline[inputoffset++];
				outputcurrentline[outputoffset++] = scanbufcurrentline[inputoffset++];
				outputcurrentline[outputoffset++] = 255;
			}
		}
		
		scanbufcurrentline += 3 * cinfo.output_width;
		lines_in_scanbuf--;
		lines_in_buf++;
	}
	
	scaler->f->end(scaler);
	((gl_object *)scaler)->f->unref((gl_object *)scaler);
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(scanbuf);
	free(row_pointers);
	fclose(f);
	
	return buffer;
}