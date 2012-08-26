#include "vc.h"

static void est_timings(FILE *f, struct est_timings *t, int n)
{
	int i;
	for(i = 0; i < n; i++){
		fprintf(f, "est%d: t1=%02x, t2=%02x\n", i, t[i].t1, t[i].t2);
	}
}

static void std_timings(FILE *f, struct std_timing *t, int n)
{
	int i;
	for(i = 0; i < n; i++){
		fprintf(f, "std%d: hszie %d, vfreq_aspecdt %d\n", i, 
			t->hsize*8+248,t->vfreq_aspect);
	}
}

static void detailed_pixel_timings(FILE *f, struct detailed_pixel_timing *t)
{
	fprintf(f, "	hactive_lo:%d\n", t->hactive_lo);
	fprintf(f, "	hblank_lo:%d\n", t->hblank_lo);
	fprintf(f, "	hactive_hblank_hi:%d\n", t->hactive_hblank_hi);
	fprintf(f, "	vactive_lo:%d\n", t->vactive_lo);
	fprintf(f, "	vblank_lo:%d\n", t->vblank_lo);
	fprintf(f, "	vactive_vblank_hi:%d\n", t->vactive_vblank_hi);
	fprintf(f, "	hsync_offset_lo:%d\n", t->hsync_offset_lo);
	fprintf(f, "	hsync_pulse_width_lo:%d\n", t->hsync_pulse_width_lo);
	fprintf(f, "	vsync_offset_pulse_width_lo:%d\n", 
		t->vsync_offset_pulse_width_lo);
	fprintf(f, "	hsync_vsync_offset_pulse_width_hi:%d\n", 
		t->hsync_vsync_offset_pulse_width_hi);
	fprintf(f, "	width_mm_lo:%d\n", t->width_mm_lo);
	fprintf(f, "	height_mm_lo:%d\n", t->height_mm_lo);
	fprintf(f, "	width_height_mm_hi:%d\n", t->width_height_mm_hi);
	fprintf(f, "	hborder:%d\n", t->hborder);
	fprintf(f, "	vborder:%d\n", t->vborder);
	fprintf(f, "	misc:%d\n", t->misc);
}

static void detailed_timings(FILE *f, struct detailed_timing *t, int n)
{
	int i;
	for(i = 0; i < n; i++){
		fprintf(f, "detailed%d: clock %d0Khz\n", i, t->pixel_clock);
		detailed_pixel_timings(f, &t->data.pixel_data);
	}
}

void printedid(FILE *f, struct edid *e)
{
	int i;

	for(i = 0; i < 8 && e->header[i]; i++)
		fprintf(f, "%c", e->header[i]);
	fprintf(f, "\n");
	fprintf(f, "mgf_id %02x%02x prod_code %04x\n",
		e->mfg_id[0], e->mfg_id[1],
		EDID_PRODUCT_ID(e));		 
	fprintf(f, "Serial %u\n", (unsigned int)e->serial);
	fprintf(f, "mfg_week: %02x\n", e->mfg_week);
	fprintf(f, "mfg_year: %02x\n", e->mfg_year);
	
	fprintf(f, "version: %02x\n", e->version);
	fprintf(f, "revision: %02x\n", e->revision);
	
	fprintf(f, "input: %02x\n", e->input);
	fprintf(f, "width_cm: %02x\n", e->width_cm);
	fprintf(f, "height_cm: %02x\n", e->height_cm);
	fprintf(f, "gamma: %02x\n", e->gamma);
	fprintf(f, "features: %02x\n", e->features);
	
	fprintf(f, "red_green_lo: %02x\n", e->red_green_lo);
	fprintf(f, "black_white_lo: %02x\n", e->black_white_lo);
	fprintf(f, "red_x: %02x\n", e->red_x);
	fprintf(f, "red_y: %02x\n", e->red_y);
	fprintf(f, "green_x: %02x\n", e->green_x);
	fprintf(f, "green_y: %02x\n", e->green_y);
	fprintf(f, "blue_x: %02x\n", e->blue_x);
	fprintf(f, "blue_y: %02x\n", e->blue_y);
	fprintf(f, "white_x: %02x\n", e->white_x);
	fprintf(f, "white_y: %02x\n", e->white_y);
	
	est_timings(f, &e->established_timings, 1);

	std_timings(f, e->standard_timings, 8);
	
	detailed_timings(f, e->detailed_timings, 4);
	
	fprintf(f, "extensions: %02x\n", e->extensions);
	
	fprintf(f, "checksum: %02x\n", e->checksum);
}

