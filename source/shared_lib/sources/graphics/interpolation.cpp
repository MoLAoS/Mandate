// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "interpolation.h"

#include <cassert>
#include <algorithm>

#include "model.h"
#include "timer.h"
#include "leak_dumper.h"

using std::min;
using std::cout;
using std::endl;

namespace Shared { namespace Graphics {

// macro to assert x is a multiple of 16, ie, x = 16n, for some natural number n
#define ASSERT_16n(x) assert((size_t(x) & 0xF) == 0)

/** Interpolate Vec3f arrays using SSE2, dest[n] = srcA[n] + dt * (srcB[n] - srcA[n]).
  * Arrays must be on a 16 byte aligned address and padded to a 16 byte multiple
  */
void interpolate(Vec3f *dest, const Vec3f *srcA, const Vec3f *srcB, float dt, size_t num) {
	// all arrays must must begin on 16 byte boundary
	ASSERT_16n(dest); ASSERT_16n(srcA); ASSERT_16n(srcB);
	assert(dt >= 0.f && dt <= 1.f); // dt should be between 0.f and 1.f inclusive
	assert(num > 0); // must be some vertices to interpolate

	// number of floats to interpolate
	const unsigned nf = num * 3;

	// just avoiding ugly casts later...
	const float *base_array = srcA[0].raw;
	const float *next_array = srcB[0].raw;
	float *dest_array = dest[0].raw;
	
	// load dt and declare 3 'scratch' __m128 vars
	const __m128 t = _mm_load1_ps(&dt);
	register __m128 base, next, diff;

	// interpolate
	for (unsigned i=0; i < nf; i += 4) {
		base = _mm_load_ps(&base_array[i]);
		next = _mm_load_ps(&next_array[i]);
		diff = _mm_sub_ps(next, base);
		diff = _mm_mul_ps(diff, t);
		next = _mm_add_ps(base, diff);
		_mm_store_ps(&dest_array[i], next);
	}
}

void test_interpolate() {
	const unsigned num_trials = 220;
	const unsigned num_verts = 500;

	Vec3f *srcA = allocate_aligned_vec3_array(num_verts);
	Vec3f *srcB = allocate_aligned_vec3_array(num_verts);
	Vec3f *result1 = allocate_aligned_vec3_array(num_verts);
	Vec3f *result2 = allocate_aligned_vec3_array(num_verts);
	
	for (unsigned i = 0; i < num_verts; ++i) {
		float f1 = float(i) / num_verts;
		float f2 = 1.f - f1;
		float f3 = float(i);
		srcA[i] = Vec3f(f1, f2, f3);
		srcB[i] = Vec3f(f2, f3, f1);
	}

	cout << "Interpolating " << num_verts << " Vec3f, "  << num_trials << " times,\n";

	Platform::Chrono chrono1, chrono2;
	chrono1.start();
	for (unsigned i = 0; i < num_trials; ++i) {
		interpolate(result1, srcA, srcB, 0.5f, num_verts);
	}
	chrono1.stop();
	cout << "\tWith SSE2, took: " << chrono1.getAccumTime() << " ticks.\n";
	
	chrono2.start();
	for (unsigned i = 0; i < num_trials; ++i) {
		Vec3f::lerpArray(result2, srcA, srcB, 0.5f, num_verts);
	}
	chrono2.stop();
	cout << "\tWith x87-FPU, took: " << chrono2.getAccumTime() << " ticks.\n";

	int diff = 0;
	for (unsigned i = 0; i < num_verts; ++i) {
		if (result1[i] != result2[i]) {
			++diff;
		}
	}
	cout << "Num diff == " << diff << endl;

	free_aligned_vec3_array(srcA);
	free_aligned_vec3_array(srcB);
	free_aligned_vec3_array(result1);
	free_aligned_vec3_array(result2);
}

// =====================================================
// class InterpolationData
// =====================================================

InterpolationData::InterpolationData(const Mesh *mesh) {
	vertices = NULL;
	normals = NULL;

	this->mesh = mesh;

	if(mesh->getFrameCount() > 1) {
		if (use_simd_interpolation) {
			vertices = allocate_aligned_vec3_array(mesh->getVertexCount());
			normals = allocate_aligned_vec3_array(mesh->getVertexCount());
		} else {
			vertices = new Vec3f[mesh->getVertexCount()];
			normals = new Vec3f[mesh->getVertexCount()];
		}
	}
}

InterpolationData::~InterpolationData() {
	if (use_simd_interpolation) {
		if (vertices) {
			free_aligned_vec3_array(vertices);
			free_aligned_vec3_array(normals);
		}
	} else {
		delete [] vertices;
		delete [] normals;
	}
}

void InterpolationData::update(float t, bool cycle) {
	updateVertices(t, cycle);
	updateNormals(t, cycle);
}

void InterpolationData::updateVertices(float t, bool cycle) {
	assert(t >= 0.0f && t <= 1.0f);
	uint32 frameCount = mesh->getFrameCount();
	uint32 vertexCount = mesh->getVertexCount();

	if (frameCount > 1 && vertexCount) {
		// calculate key-frames to use
		uint32 prevFrame = min<uint32>(static_cast<uint32>(t * frameCount), frameCount - 1);
		uint32 nextFrame = cycle ? (prevFrame + 1) % frameCount : min(prevFrame + 1, frameCount - 1);

		// assert sanity
		assert(prevFrame >= 0 && prevFrame < frameCount);
		assert(nextFrame >= 0 && nextFrame < frameCount);

		// convert 'global' t (0-1 for entire anim) to local t (0-1 between two frames)
		float localT = t * frameCount - prevFrame;
		assert(localT >= 0.f && localT <= 1.f);

		//interpolate vertices
		if (use_simd_interpolation) {
			const Vec3f *srcA = mesh->getVertArray(prevFrame);
			const Vec3f *srcB = mesh->getVertArray(nextFrame);
			interpolate(vertices, srcA, srcB, localT, vertexCount);
		} else {
			uint32 prevFrameBase = prevFrame * vertexCount;
			uint32 nextFrameBase = nextFrame * vertexCount;
			const Vec3f *srcA = &mesh->getVertices()[prevFrameBase];
			const Vec3f *srcB = &mesh->getVertices()[nextFrameBase];
			Vec3f::lerpArray(vertices, srcA, srcB, localT, vertexCount);
		}
	}
}

void InterpolationData::updateNormals(float t, bool cycle) {
	assert(t >= 0.0f && t <= 1.0f);

	uint32 frameCount = mesh->getFrameCount();
	uint32 vertexCount = mesh->getVertexCount();

	if (frameCount > 1 && vertexCount) {
		//misc vars
		uint32 prevFrame = min<uint32>(static_cast<uint32>(t * frameCount), frameCount - 1);
		uint32 nextFrame = cycle ? (prevFrame + 1) % frameCount : min(prevFrame + 1, frameCount - 1);

		//assertions
		assert(prevFrame >= 0 && prevFrame < frameCount);
		assert(nextFrame >= 0 && nextFrame < frameCount);

		float localT = t * frameCount - prevFrame;

		//interpolate vertices
		if (use_simd_interpolation) {
			const Vec3f *srcA = mesh->getNormArray(prevFrame);
			const Vec3f *srcB = mesh->getNormArray(nextFrame);
			interpolate(normals, srcA, srcB, localT, vertexCount);
		} else {
			uint32 prevFrameBase = prevFrame * vertexCount;
			uint32 nextFrameBase = nextFrame * vertexCount;
			const Vec3f *srcA = &mesh->getNormals()[prevFrameBase];
			const Vec3f *srcB = &mesh->getNormals()[nextFrameBase];
			Vec3f::lerpArray(normals, srcA, srcB, localT, vertexCount);
		}
	}
}

}}//end namespace
