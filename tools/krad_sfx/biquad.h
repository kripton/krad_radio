#ifndef BIQUAD_H
#define BIQUAD_H

#define LN_2_2 0.34657359f // ln(2)/2

#include "ladspa-util.h"

#ifndef LIMIT
#define LIMIT(v,l,u) (v<l?l:(v>u?u:v))
#endif

/* Biquad filter (adapted from lisp code by Eli Brandt,
   http://www.cs.cmu.edu/~eli/) */

typedef struct {
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;
	float x1;
	float x2;
	float y1;
	float y2;
} biquad;

static inline void biquad_init(biquad *f) {
	f->x1 = 0.0f;
	f->x2 = 0.0f;
	f->y1 = 0.0f;
	f->y2 = 0.0f;
}

static inline void eq_set_params(biquad *f, float fc, float gain, float bw,
			  float fs);
static inline void eq_set_params(biquad *f, float fc, float gain, float bw,
			  float fs)
{
	float w = 2.0f * M_PI * LIMIT(fc, 1.0f, fs/2.0f) / fs;
	float cw = cosf(w);
	float sw = sinf(w);
	float J = pow(10.0f, gain * 0.025f);
	float g = sw * sinhf(LN_2_2 * LIMIT(bw, 0.0001f, 4.0f) * w / sw);
	float a0r = 1.0f / (1.0f + (g / J));

	f->b0 = (1.0f + (g * J)) * a0r;
	f->b1 = (-2.0f * cw) * a0r;
	f->b2 = (1.0f - (g * J)) * a0r;
	f->a1 = -(f->b1);
	f->a2 = ((g / J) - 1.0f) * a0r;
}

static inline void ls_set_params(biquad *f, float fc, float gain, float slope,
			  float fs);
static inline void ls_set_params(biquad *f, float fc, float gain, float slope,
			  float fs)
{
	float w = 2.0f * M_PI * LIMIT(fc, 1.0, fs/2.0) / fs;
	float cw = cosf(w);
	float sw = sinf(w);
	float A = powf(10.0f, gain * 0.025f);
	float b = sqrt(((1.0f + A * A) / LIMIT(slope, 0.0001f, 1.0f)) - ((A -
					1.0f) * (A - 1.0)));
	float apc = cw * (A + 1.0f);
	float amc = cw * (A - 1.0f);
	float bs = b * sw;
	float a0r = 1.0f / (A + 1.0f + amc + bs);

	f->b0 = a0r * A * (A + 1.0f - amc + bs);
	f->b1 = a0r * 2.0f * A * (A - 1.0f - apc);
	f->b2 = a0r * A * (A + 1.0f - amc - bs);
	f->a1 = a0r * 2.0f * (A - 1.0f + apc);
	f->a2 = a0r * (-A - 1.0f - amc + bs);
}

static inline void hs_set_params(biquad *f, float fc, float gain, float slope,
			  float fs);
static inline void hs_set_params(biquad *f, float fc, float gain, float slope,
			  float fs)
{
	float w = 2.0f * M_PI * LIMIT(fc, 1.0, fs/2.0) / fs;
	float cw = cosf(w);
	float sw = sinf(w);
	float A = powf(10.0f, gain * 0.025f);
	float b = sqrt(((1.0f + A * A) / LIMIT(slope, 0.0001f, 1.0f)) - ((A -
					1.0f) * (A - 1.0f)));
	float apc = cw * (A + 1.0f);
	float amc = cw * (A - 1.0f);
	float bs = b * sw;
	float a0r = 1.0f / (A + 1.0f - amc + bs);

	f->b0 = a0r * A * (A + 1.0f + amc + bs);
	f->b1 = a0r * -2.0f * A * (A - 1.0f + apc);
	f->b2 = a0r * A * (A + 1.0f + amc - bs);
	f->a1 = a0r * -2.0f * (A - 1.0f - apc);
	f->a2 = a0r * (-A - 1.0f + amc + bs);
}

static inline void lp_set_params(biquad *f, float fc, float bw, float fs)
{
	float omega = 2.0 * M_PI * fc/fs;
	float sn = sin(omega);
	float cs = cos(omega);
	float alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

        const float a0r = 1.0 / (1.0 + alpha);
        f->b0 = a0r * (1.0 - cs) * 0.5;
	f->b1 = a0r * (1.0 - cs);
        f->b2 = a0r * (1.0 - cs) * 0.5;
        f->a1 = a0r * (2.0 * cs);
        f->a2 = a0r * (alpha - 1.0);
}

static inline void hp_set_params(biquad *f, float fc, float bw, float fs)
{
	float omega = 2.0 * M_PI * fc/fs;
	float sn = sin(omega);
	float cs = cos(omega);
	float alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

        const float a0r = 1.0 / (1.0 + alpha);
        f->b0 = a0r * (1.0 + cs) * 0.5;
        f->b1 = a0r * -(1.0 + cs);
        f->b2 = a0r * (1.0 + cs) * 0.5;
        f->a1 = a0r * (2.0 * cs);
        f->a2 = a0r * (alpha - 1.0);
}

static inline void bp_set_params(biquad *f, float fc, float bw, float fs)
{
	float omega = 2.0 * M_PI * fc/fs;
	float sn = sin(omega);
	float cs = cos(omega);
	float alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

        const float a0r = 1.0 / (1.0 + alpha);
        f->b0 = a0r * alpha;
        f->b1 = 0.0;
        f->b2 = a0r * -alpha;
        f->a1 = a0r * (2.0 * cs);
        f->a2 = a0r * (alpha - 1.0);
}

static inline float biquad_run(biquad *f, const float x) {

	float y;

	y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
		      + f->a1 * f->y1 + f->a2 * f->y2;
	y = flush_to_zero(y);
	f->x2 = f->x1;
	f->x1 = x;
	f->y2 = f->y1;
	f->y1 = y;

	return y;
}

static inline float biquad_run_fb(biquad *f, float x, const float fb) {

	float y;

	x += f->y1 * fb * 0.98;
	y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
		      + f->a1 * f->y1 + f->a2 * f->y2;
	y = flush_to_zero(y);
	f->x2 = f->x1;
	f->x1 = x;
	f->y2 = f->y1;
	f->y1 = y;

	return y;
}

#endif

