/*
math-neon:  ARM Neon VFP Optimised Math Library based on cmath
Contact:    lachlan.ts@gmail.com
Copyright (C) 2009  Lachlan Tychsen - Smith aka Adventus

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "math.h"
#include "math_neon.h"

const float __logf_range =  0.693147180f;
const float __logf_invrange = 1.4426950f;

const float __logf_lut[4] = {
	0.14154254546192570,	//p3
	2.30888151926361070,	//p1
	-0.8681945310048997,	//p2
	-1.5822295331515284 	//p0
};

float logf_c(float x)
{
	float a, b;
	int m;
	
	union {
		float   f;
		int 	i;
	} r;
			
	r.f = x;
	m = (r.i >> 23) - 127;
	r.i = (r.i & 0x007FFFFF) | 0x3F800000; // 127 << 23
	
	//Taylor Polynomial (Estrins)
	a = (__logf_lut[0] * r.f) + (__logf_lut[2]);
	b = (__logf_lut[1] * r.f) + (__logf_lut[3]);
	r.f = r.f * r.f;
	r.f = b + a * r.f;
	r.f += ((float) m) * __logf_range;

	return r.f;
}

float logf_neon(float x)
{
#ifdef __MATH_NEON
	float r;
	volatile asm (
	"vdup.f32 		d0, %1					\n\t"	//d0 = {x, x}
	"vabs.f32 		d1, d0					\n\t"	//d1 = {ax, ax}
	
	//Range Reduction:
	"vdup.f32 		d3, %3					\n\t"	//d3 = {__sinf_invrange, __sinf_invrange}
	"vmul.f32 		d2, d1, d3				\n\t"	//d2 = d1 * d3 
	"vcvt.u32.f32 	d2, d2					\n\t"	//d2 = (int) d2
	"vcvt.f32.u32 	d4, d2					\n\t"	//d4 = (float) d2
	"vdup.f32 		d5, %2					\n\t"	//d5 = {__sinf_range, __sinf_range}
	"vmls.f32 		d1, d4, d5				\n\t"	//d1 = d1 - d4 * d5
	
	//Checking Quadrant:
	//ax = ax - (k&1) * M_PI_2
	"vand.i32 		d3, d2, #0x00000001		\n\t"	//d3 = d2 & 0x1
	"vcvt.f32.u32 	d4, d3					\n\t"	//d4 = (float) d3
	"vmls.f32 		d1, d5, d4				\n\t"	//d1 = d1 - d5 * d4
	
	//ax = ax ^ ((k & 1) ^ (k & 2 >> 1) ^ (x < 0) << 31)
	"vand.i32 		d4, d2, #0x00000002		\n\t"	//d4 = d2 & 0x2
	"vshr.u32 		d4, d4, #1				\n\t"	//d4 = d4 >> 1
	"vclt.f32 		d5, d0, #0				\n\t"	//d5 = (d0 < 0.0)
	"veor.i32 		d5, d5, d4				\n\t"	//d5 = d5 ^ d4
	"veor.i32 		d5, d5, d3				\n\t"	//d5 = d5 ^ d3
	"vshl.i32 		d5, d5, #31				\n\t"	//d5 = d5 << 31
	"veor.i32 		d1, d1, d5				\n\t"	//d1 = d1 ^ d5
	
	//polynomial:
	"vmul.f32 		d2, d1, d1				\n\t"	//d2 = d1*d1 = {x^2, x^2}	
	"vld1.32 		{d4, d5}, [%4]			\n\t"	//d4 = {p7, p3}, d5 = {p5, p1}
	"vmul.f32 		d3, d2, d2				\n\t"	//d3 = d2*d2 = {x^4, x^4}		
	"vmul.f32 		q0, q2, d1[0]			\n\t"	//q0 = q2 * d1[0] = {p7x, p3x, p5x, p1x}
	"vmla.f32 		d1, d0, d2[0]			\n\t"	//d1 = d1 + d0*d2 = {p5x + p7x^3, p1x + p3x^3}		
	"vmla.f32 		d1, d3, d1[0]			\n\t"	//d1 = d1 + d3*d0 = {p5x + p7x^3 + p5x^5 + p7x^7, p1x + p3x^3 + p5x^5 + p7x^7}		
	"vmov.f32 		%0, s3					\n\t"	//r = s0
	
	: "=r"(r)
	: "g"(x), "g"(__sinf_range), "g"(__sinf_invrange), "g"(__sinf_lut) 
    : "d0", "d1", "d2", "d3", "d4", "d5"
	);
	return r;
#else
	return expf_c(x);
#endif
}
