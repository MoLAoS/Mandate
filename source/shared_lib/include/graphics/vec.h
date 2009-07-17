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

#ifndef _SHARED_GRAPHICS_VEC_H_
#define _SHARED_GRAPHICS_VEC_H_

#include <cmath>
#include "simd.h"

namespace Shared { namespace Graphics {

template<typename T> class Vec2;
template<typename T> class Vec3;
template<typename T> class Vec4;

// =====================================================
// class Vec2
// =====================================================

template<typename T> class Vec2 {
public:
	T x;
	T y;

	Vec2() {}

	explicit Vec2(T *p) {
		this->x = p[0];
		this->y = p[1];
	}

	explicit Vec2(T xy) {
		this->x = xy;
		this->y = xy;
	}

	template<typename S> explicit Vec2(const Vec2<S> &v) {
		this->x = v.x;
		this->y = v.y;
	}

	Vec2(T x, T y) {
		this->x = x;
		this->y = y;
	}

	Vec2 &operator =(const Vec2<T> &v) {
		x = v.x;
		y = v.y;
		return (*this);
	}

	T *ptr() {
		return reinterpret_cast<T*>(this);
	}

	const T *ptr() const {
		return reinterpret_cast<const T*>(this);
	}

	bool operator ==(const Vec2<T> &v) const {
		return x == v.x && y == v.y;
	}

	bool operator !=(const Vec2<T> &v) const {
		return x != v.x || y != v.y;
	}

   bool operator < ( const Vec2<T> &v ) const
   {
      return x < v.x || ( x == v.x && y < v.y );
   }

	Vec2<T> operator +(const Vec2<T> &v) const {
		return Vec2(x + v.x, y + v.y);
	}

	Vec2<T> operator -(const Vec2<T> &v) const {
		return Vec2(x -v.x, y - v.y);
	}

	Vec2<T> operator -() const {
		return Vec2(-x, -y);
	}

	Vec2<T> operator *(const Vec2<T> &v) const {
		return Vec2(x*v.x, y*v.y);
	}

	Vec2<T> operator *(T s) const {
		return Vec2(x * s, y * s);
	}

	Vec2<T> operator /(const Vec2<T> &v) const {
		return Vec2(x / v.x, y / v.y);
	}

	Vec2<T> operator /(T s) const {
		return Vec2(x / s, y / s);
	}

	Vec2<T> operator +=(const Vec2<T> &v) {
		x += v.x;
		y += v.y;
		return *this;
	}

	Vec2<T> operator -=(const Vec2<T> &v) {
		x -= v.x;
		y -= v.y;
		return *this;
	}

	Vec2<T> lerp(T t, const Vec2<T> &v) const {
		return *this + (v - *this)*t;
	}

	T dot(const Vec2<T> &v) const {
		return x*v.x + y*v.y;
	}

	float dist(const Vec2<T> &v) const {
		return Vec2<T>(v - *this).length();
	}

	float length() const {
		return sqrtf(static_cast<float>(x * x + y * y));
	}

	void normalize() {
		T m = length();
		x /= m;
		y /= m;
	}
	
	static void lerpArray(Vec2 *dest, const Vec2 *srcA, const Vec2 *srcB, float t, size_t size) {
		for(int i = 0; i < size; ++i) {
			dest[i] = srcB[i].lerp(t, srcA[i]);
		}
	}
};

// =====================================================
// class Vec3
// =====================================================

ALIGN_VEC12_DECL template<typename T> class Vec3 {
public:
	T x;
	T y;
	T z;

	Vec3() {}

	explicit Vec3(T *p) {
		this->x = p[0];
		this->y = p[1];
		this->z = p[2];
	}

	explicit Vec3(T xyz) {
		this->x = xyz;
		this->y = xyz;
		this->z = xyz;
	}

	template<typename S> explicit Vec3(const Vec3<S> &v) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
	}

	Vec3(T x, T y, T z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	explicit Vec3(Vec4<T> v) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
	}

	Vec3 &operator =(const Vec3<T> &v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return (*this);
	}

	T *ptr() {
		return reinterpret_cast<T*>(this);
	}

	const T *ptr() const {
		return reinterpret_cast<const T*>(this);
	}

	bool operator ==(const Vec3<T> &v) const {
		return x == v.x && y == v.y && z == v.z;
	}

	bool operator !=(const Vec3<T> &v) const {
		return x != v.x || y != v.y || z != v.z;
	}

	Vec3<T> operator +(const Vec3<T> &v) const {
		return Vec3(x + v.x, y + v.y, z + v.z);
	}

	Vec3<T> operator -(const Vec3<T> &v) const {
		return Vec3(x -v.x, y - v.y, z - v.z);
	}

	Vec3<T> operator -() const {
		return Vec3(-x, -y, -z);
	}

	Vec3<T> operator *(const Vec3<T> &v) const {
		return Vec3(x*v.x, y*v.y, z*v.z);
	}

	Vec3<T> operator *(T s) const {
		return Vec3(x*s, y*s, z*s);
	}

	Vec3<T> operator /(const Vec3<T> &v) const {
		return Vec3(x / v.x, y / v.y, z / v.z);
	}

	Vec3<T> operator /(T s) const {
		return Vec3(x / s, y / s, z / s);
	}

	Vec3<T> operator +=(const Vec3<T> &v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Vec3<T> operator -=(const Vec3<T> &v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Vec3<T> lerp(T t, const Vec3<T> &v) const {
		return *this + (v - *this) * t;
	}

	T dot(const Vec3<T> &v) const {
		return x * v.x + y * v.y + z * v.z;
	}

	float dist(const Vec3<T> &v) const {
		return Vec3<T>(v - *this).length();
	}

	float length() const {
		return sqrtf(static_cast<float>(x * x + y * y + z * z));
	}

	void normalize() {
		T m = length();
		x /= m;
		y /= m;
		z /= m;
	}

	Vec3<T> getNormalized() const {
		T m = length();
		return Vec3<T>(x / m, y / m, z / m);
	}

	Vec3<T> cross(const Vec3<T> &v) const {
		return Vec3<T>(
				   y * v.z - z * v.y,
				   z * v.x - x * v.z,
				   x * v.y - y * v.x);
	}

	Vec3<T> normal(const Vec3<T> &p1, const Vec3<T> &p2) const {
		Vec3<T> rv;
		rv = (p2 - *this).cross(p1 - *this);
		rv.normalize();
		return rv;
	}

	Vec3<T> normal(const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3, const Vec3<T> &p4) const {
		Vec3<T> rv;

		rv = this->normal(p1, p2) + this->normal(p2, p3) + this->normal(p3, p4)	+ this->normal(p4, p1);
		rv.normalize();
		return rv;
	}

	static void lerpArray(Vec3 *dest, const Vec3 *srcA, const Vec3 *srcB, float t, size_t size) {
		for(int i = 0; i < size; ++i) {
			dest[i] = srcA[i].lerp(t, srcB[i]);
		}
	}

#ifdef ALIGN_12BYTE_VECTORS
	static void* operator new(size_t size)		{return _mm_malloc(size, 16);}
	static void* operator new[](size_t size)	{return _mm_malloc(size, 16);}
	static void operator delete(void* ptr)		{_mm_free(ptr);}
	static void operator delete[](void* ptr)	{_mm_free(ptr);}
#endif

} ALIGN_VEC12_ATTR;

// =====================================================
// class Vec4
// =====================================================

ALIGN_VEC_DECL template<typename T> class Vec4 {
public:
	T x;
	T y;
	T z;
	T w;

	Vec4() {}

	explicit Vec4(T *p) {
		this->x = p[0];
		this->y = p[1];
		this->z = p[2];
		this->w = p[3];
	}

	explicit Vec4(T xyzw) {
		this->x = xyzw;
		this->y = xyzw;
		this->z = xyzw;
		this->w = xyzw;
	}

	template<typename S> explicit Vec4(const Vec4<S> &v) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = v.w;
	}

	Vec4(T x, T y, T z, T w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	Vec4(Vec3<T> v, T w) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = w;
	}

	explicit Vec4(Vec3<T> v) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = 1;
	}

	Vec4 &operator =(const Vec4<T> &v) {
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return (*this);
	}

	T *ptr() {
		return reinterpret_cast<T*>(this);
	}

	const T *ptr() const {
		return reinterpret_cast<const T*>(this);
	}

	bool operator ==(const Vec4<T> &v) const {
		return x == v.x && y == v.y && z == v.z && w == v.w;
	}

	bool operator !=(const Vec4<T> &v) const {
		return x != v.x || y != v.y || z != v.z || w != v.w;
	}

	Vec4<T> operator +(const Vec4<T> &v) const {
		return Vec4(x + v.x, y + v.y, z + v.z, w + v.w);
	}

	Vec4<T> operator -(const Vec4<T> &v) const {
		return Vec4(x -v.x, y - v.y, z - v.z, w - v.w);
	}

	Vec4<T> operator -() const {
		return Vec4(-x, -y, -z, -w);
	}

	Vec4<T> operator *(const Vec4<T> &v) const {
		return Vec4(x*v.x, y*v.y, z*v.z, w*v.w);
	}

	Vec4<T> operator *(T s) const {
		return Vec4(x*s, y*s, z*s, w*s);
	}

	Vec4<T> operator /(const Vec4<T> &v) const {
		return Vec4(x / v.x, y / v.y, z / v.z, w / v.w);
	}

	Vec4<T> operator /(T s) const {
		return Vec4(x / s, y / s, z / s, w / s);
	}

	Vec4<T> operator +=(const Vec4<T> &v) {
		x += v.x;
		y += v.y;
		z += v.z;
		w += w.z;
		return *this;
	}

	Vec4<T> operator -=(const Vec4<T> &v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= w.z;
		return *this;
	}

	Vec4<T> lerp(T t, const Vec4<T> &v) const {
		return *this + (v - *this) *t;
	}

	T dot(const Vec4<T> &v) const {
		return x*v.x + y*v.y + z*v.z + w*v.w;
	}

	static void lerpArray(Vec4 *dest, const Vec4 *srcA, const Vec4 *srcB, float t, size_t size) {
		for(int i = 0; i < size; ++i) {
			dest[i] = srcB[i].lerp(t, srcA[i]);
		}
	}

#ifdef ALIGN_VECTORS
	static void* operator new(size_t size)		{return _mm_malloc(size, 16);}
	static void* operator new[](size_t size)	{return _mm_malloc(size, 16);}
	static void operator delete(void* ptr)		{_mm_free(ptr);}
	static void operator delete[](void* ptr)	{_mm_free(ptr);}
#endif

} ALIGN_VEC_ATTR;

typedef Vec2<int> Vec2i;
typedef Vec3<int> Vec3i;
typedef Vec4<int> Vec4i;

typedef Vec2<bool> Vec2b;
typedef Vec3<bool> Vec3b;
typedef Vec4<bool> Vec4b;

typedef Vec2<char> Vec2c;
typedef Vec3<char> Vec3c;
typedef Vec4<char> Vec4c;

typedef Vec2<float> Vec2f;

typedef Vec2<double> Vec2d;
typedef Vec3<double> Vec3d;
typedef Vec4<double> Vec4d;

#ifndef USE_SSE2_INTRINSICS
typedef Vec3<float> Vec3f;
typedef Vec4<float> Vec4f;
#else

//SSE2 16-byte aligned implementations

#define _mm_ps_this_v_op_equal_this_ret(func)							\
		__m128 a = _mm_loadu_ps(reinterpret_cast<const float*>(this));	\
		__m128 b = _mm_loadu_ps(reinterpret_cast<const float*>(&v));	\
		a = func(a, b);													\
		_mm_storeu_ps(reinterpret_cast<float*>(this), a);				\
		return *this;

#define _mm_ps_this_v_dest_op(func)									\
		__m128 a = _mm_loadu_ps(reinterpret_cast<const float*>(this));	\
		__m128 b = _mm_loadu_ps(reinterpret_cast<const float*>(&v));	\
		a = func(a, b);													\
		_mm_storeu_ps(reinterpret_cast<float*>(&dest), a);

#define _mm_ss_this_v_op_equal_this_ret(func)							\
		__m128 a = _mm_loadu_ps(reinterpret_cast<const float*>(this));	\
		__m128 b = _mm_loadu_ps(reinterpret_cast<const float*>(&v));	\
		a = func(a, b);													\
		_mm_storeu_ps(reinterpret_cast<float*>(this), a);				\
		return *this;

#define _mm_ss_this_v_dest_op(func)							\
		__m128 a = _mm_loadu_ps(reinterpret_cast<const float*>(this));	\
		__m128 b = _mm_loadu_ps(reinterpret_cast<const float*>(&v));	\
		a = func(a, b);													\
		_mm_storeu_ps(reinterpret_cast<float*>(&dest), a);

class Vec3f;
class Vec4f;
typedef ALIGN_VEC_DECL float  ALIGN_VEC_ATTR AlignedFloat;
typedef ALIGN_VEC_DECL int ALIGN_VEC_ATTR AlignedInt;

ALIGN_VEC_DECL class SSE2Vec4f {
public:
	float x;
	float y;
	float z;
	float w;
	
	/** Default constructor with no initialization. */
	SSE2Vec4f() {}

	/** Create from a 16-byte aligned array of 4 floats */
	explicit SSE2Vec4f(const AlignedFloat *p) {
/*		x = p[0];
		y = p[1];
		z = p[2];
		w = p[3];*/
		__m128 a = _mm_loadu_ps(p);
		_mm_storeu_ps(reinterpret_cast<float*>(this), a);
	}

	explicit SSE2Vec4f(AlignedFloat xyzw) {
/*		x = xyzw;
		y = xyzw;
		z = xyzw;
		w = xyzw;*/
		__m128 a = _mm_load1_ps(&xyzw);
		_mm_storeu_ps(reinterpret_cast<float*>(this), a);
	}

	float *ptr() {
		return reinterpret_cast<float*>(this);
	}

	const float *ptr() const {
		return reinterpret_cast<const float*>(this);
	}
/*
	operator Vec3f *() {
		return reinterpret_cast<Vec3f*>(this);
	}

	operator Vec4f *() {
		return reinterpret_cast<Vec4f*>(this);
	}

	operator const Vec3f *() {
		return reinterpret_cast<const Vec3f*>(this);
	}

	operator const Vec4f *() {
		return reinterpret_cast<const Vec4f*>(this);
	}
*/
	static void* operator new(size_t size)		{return _mm_malloc(size, 16);}
	static void* operator new[](size_t size)	{return _mm_malloc(size, 16);}
	static void operator delete(void* ptr)		{_mm_free(ptr);}
	static void operator delete[](void* ptr)	{_mm_free(ptr);}
protected:
	// assignment operators
	SSE2Vec4f &operator =(const SSE2Vec4f &v) {
/*		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = v.w;*/
		__m128 a;
		a = _mm_loadu_ps(reinterpret_cast<const float*>(&v));
		_mm_storeu_ps(reinterpret_cast<float*>(this), a);
		return *this;
	}

	SSE2Vec4f &operator +=(const SSE2Vec4f &v) {
/*		this->x += v.x;
		this->y += v.y;
		this->z += v.z;
		this->w += v.w;
		return *this;*/
		_mm_ps_this_v_op_equal_this_ret(_mm_add_ps);
	}

	SSE2Vec4f &operator -=(const SSE2Vec4f &v) {
/*		this->x -= v.x;
		this->y -= v.y;
		this->z -= v.z;
		this->w -= v.w;
		return *this;*/
		_mm_ps_this_v_op_equal_this_ret(_mm_sub_ps);
	}

	SSE2Vec4f &operator *=(const SSE2Vec4f &v) {
/*		this->x /= v.x;
		this->y /= v.y;
		this->z /= v.z;
		this->w /= v.w;
		return *this;*/
		_mm_ps_this_v_op_equal_this_ret(_mm_mul_ps);
	}

	SSE2Vec4f &operator /=(const SSE2Vec4f &v) {
/*		this->x *= v.x;
		this->y *= v.y;
		this->z *= v.z;
		this->w *= v.w;
		return *this;*/
		_mm_ps_this_v_op_equal_this_ret(_mm_div_ps);
	}

	void operator_add(SSE2Vec4f &dest, const SSE2Vec4f &v) const {
/*		dest.x = this->x + v.x;
		dest.y = this->y + v.y;
		dest.z = this->z + v.z;
		dest.w = this->w + v.w;*/
		_mm_ps_this_v_dest_op(_mm_add_ps);
	}

	void operator_sub(SSE2Vec4f &dest, const SSE2Vec4f &v) const {
/*		dest.x = this->x - v.x;
		dest.y = this->y - v.y;
		dest.z = this->z - v.z;
		dest.w = this->w - v.w;*/
		_mm_ps_this_v_dest_op(_mm_sub_ps);
	}

	void operator_mul(SSE2Vec4f &dest, const SSE2Vec4f &v) const {
/*		dest.x = this->x * v.x;
		dest.y = this->y * v.y;
		dest.z = this->z * v.z;
		dest.w = this->w * v.w;*/
		_mm_ps_this_v_dest_op(_mm_mul_ps);
	}

	void operator_div(SSE2Vec4f &dest, const SSE2Vec4f &v) const {
/*		dest.x = this->x / v.x;
		dest.y = this->y / v.y;
		dest.z = this->z / v.z;
		dest.w = this->w / v.w;*/
		_mm_ps_this_v_dest_op(_mm_div_ps);
	}

	void lerp(SSE2Vec4f &dest, AlignedFloat t, const SSE2Vec4f &v) const {
		__m128 a = _mm_load1_ps(&t);
		__m128 b = _mm_loadu_ps(reinterpret_cast<const float*>(this));
		__m128 c = _mm_loadu_ps(reinterpret_cast<const float*>(&v));
		c = _mm_sub_ps(c, b);
		c = _mm_mul_ps(c, a);
		c = _mm_add_ps(b, c);
		_mm_storeu_ps(reinterpret_cast<float*>(&dest), c);
		//return *this + (v - *this) * t;
	}

	static void lerpArray(SSE2Vec4f *dest, const SSE2Vec4f *srcA, const SSE2Vec4f *srcB, AlignedFloat t, size_t size) {
		__m128 a = _mm_load1_ps(&t);
		for(int i = 0; i < size; ++i) {
			__m128 b = _mm_loadu_ps(reinterpret_cast<const float*>(&srcA[i]));
			__m128 c = _mm_loadu_ps(reinterpret_cast<const float*>(&srcB[i]));
			c = _mm_sub_ps(c, b);
			c = _mm_mul_ps(c, a);
			c = _mm_add_ps(b, c);
			_mm_storeu_ps(reinterpret_cast<float*>(&dest[i]), c);
		}
	}

} ALIGN_ATTR(16);

class Vec3f : public SSE2Vec4f {
public:
	Vec3f() {}
	explicit Vec3f(float *p) : SSE2Vec4f(p) {}
	explicit Vec3f(float xyz) : SSE2Vec4f(xyz) {}
	explicit Vec3f(const Vec4f &v);
	//Vec3f(const Vec3f &v) : SSE2Vec4f(reinterpret_cast<const AlignedFloat*>(&v)) {}


	//TODO implement for int using sse2 then leave template for others
	template<typename S> explicit Vec3f(const Vec3<S> &v) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
	}

	Vec3f(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}
	

	template<typename S> explicit Vec3f(Vec4<S> v) : Vec3<S>(v) {}

	// assignment operators
	Vec3f &operator =(const Vec3f &v) {
		return reinterpret_cast<Vec3f&>(SSE2Vec4f::operator =(v));
	}

	Vec3f &operator +=(const Vec3f &v) {
		return reinterpret_cast<Vec3f&>(SSE2Vec4f::operator +=(v));
	}

	Vec3f &operator -=(const Vec3f &v) {
		return reinterpret_cast<Vec3f&>(SSE2Vec4f::operator -=(v));
	}

	Vec3f &operator *=(const Vec3f &v) {
		return reinterpret_cast<Vec3f&>(SSE2Vec4f::operator *=(v));
	}

	Vec3f &operator /=(const Vec3f &v) {
		return reinterpret_cast<Vec3f&>(SSE2Vec4f::operator /=(v));
	}

	// binary arithmatic operators
	Vec3f operator +(const Vec3f &v) const {
		Vec3f dest;
		SSE2Vec4f::operator_add(dest, v);
		return dest;
	}

	Vec3f operator -(const Vec3f &v) const {
		Vec3f dest;
		SSE2Vec4f::operator_sub(dest, v);
		return dest;
	}

	Vec3f operator *(const Vec3f &v) const {
		Vec3f dest;
		SSE2Vec4f::operator_mul(dest, v);
		return dest;
	}

	Vec3f operator /(const Vec3f &v) const {
		Vec3f dest;
		SSE2Vec4f::operator_div(dest, v);
		return dest;
	}
	


	bool operator ==(const Vec3f &v) const {
		return x == v.x && y == v.y && z == v.z;
	}

	bool operator !=(const Vec3f &v) const {
		return x != v.x || y != v.y || z != v.z;
	}

	Vec3f operator -() const {
		return Vec3f(-x, -y, -z);
	}

	Vec3f operator *(float s) const {
		return Vec3f(x*s, y*s, z*s);
	}


	Vec3f operator /(float s) const {
		return Vec3f(x / s, y / s, z / s);
	}

	Vec3f lerp(float t, const Vec3f &v) const {
		return *this + (v - *this) * t;
	}

	float dot(const Vec3f &v) const {
		return x * v.x + y * v.y + z * v.z;
	}

	float dist(const Vec3f &v) const {
		return Vec3f(v - *this).length();
	}

	float length() const {
		return sqrtf(static_cast<float>(x * x + y * y + z * z));
	}

	void normalize() {
		float m = length();
		x /= m;
		y /= m;
		z /= m;
	}

	Vec3f getNormalized() const {
		float m = length();
		return Vec3f(x / m, y / m, z / m);
	}

	Vec3f cross(const Vec3f &v) const {
		return Vec3f(
				   y * v.z - z * v.y,
				   z * v.x - x * v.z,
				   x * v.y - y * v.x);
	}

	Vec3f normal(const Vec3f &p1, const Vec3f &p2) const {
		Vec3f rv;
		rv = (p2 - *this).cross(p1 - *this);
		rv.normalize();
		return rv;
	}

	Vec3f normal(const Vec3f &p1, const Vec3f &p2, const Vec3f &p3, const Vec3f &p4) const {
		Vec3f rv;

		rv = this->normal(p1, p2);
		rv = rv + this->normal(p2, p3);
		rv = rv + this->normal(p3, p4);
		rv = rv + this->normal(p4, p1);
		rv.normalize();
		return rv;
	}

	static void lerpArray(Vec3f *dest, const Vec3f *srcA, const Vec3f *srcB, float t, size_t size) {
		SSE2Vec4f::lerpArray(dest, srcA, srcB, t, size);
	}
};

class Vec4f : public SSE2Vec4f {
public:
	Vec4f() {}
	explicit Vec4f(float *p) : SSE2Vec4f(p) {}
	explicit Vec4f(float xyzw) : SSE2Vec4f(xyzw) {}
	Vec4f(const Vec4f &v) : SSE2Vec4f(reinterpret_cast<const AlignedFloat*>(&v)) {}

	template<typename S> explicit Vec4f(const Vec4<S> &v) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = v.w;
	}

	Vec4f(float x, float y, float z, float w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	Vec4f(const Vec3f &v, float w) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = w;
	}

	explicit Vec4f(const Vec3f &v) {
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = 1;
	}

	// assignment operators
	Vec4f &operator =(const Vec4f &v) {
		return reinterpret_cast<Vec4f&>(SSE2Vec4f::operator =(v));
	}

	Vec4f &operator +=(const Vec4f &v) {
		return reinterpret_cast<Vec4f&>(SSE2Vec4f::operator +=(v));
	}

	Vec4f &operator -=(const Vec4f &v) {
		return reinterpret_cast<Vec4f&>(SSE2Vec4f::operator -=(v));
	}

	Vec4f &operator *=(const Vec4f &v) {
		return reinterpret_cast<Vec4f&>(SSE2Vec4f::operator *=(v));
	}

	Vec4f &operator /=(const Vec4f &v) {
		return reinterpret_cast<Vec4f&>(SSE2Vec4f::operator /=(v));
	}


	// binary arithmatic operators
	Vec4f operator +(const Vec4f &v) const {
		Vec4f dest;
		SSE2Vec4f::operator_add(dest, v);
		return dest;
	}

	Vec4f operator -(const Vec4f &v) const {
		Vec4f dest;
		SSE2Vec4f::operator_sub(dest, v);
		return dest;
	}

	Vec4f operator *(const Vec4f &v) const {
		Vec4f dest;
		SSE2Vec4f::operator_mul(dest, v);
		return dest;
	}

	Vec4f operator /(const Vec4f &v) const {
		Vec4f dest;
		SSE2Vec4f::operator_div(dest, v);
		return dest;
	}


	bool operator ==(const Vec4f &v) const {
		return x == v.x && y == v.y && z == v.z && w == v.w;
	}

	bool operator !=(const Vec4f &v) const {
		return x != v.x || y != v.y || z != v.z || w != v.w;
	}
	
	Vec4f operator -() const {
		return Vec4f(-x, -y, -z, -w);
	}

	Vec4f operator *(float s) const {
		return Vec4f(x*s, y*s, z*s, w*s);
	}

	Vec4f operator /(float s) const {
		return Vec4f(x / s, y / s, z / s, w / s);
	}

	Vec4f lerp(float t, const Vec4f &v) const {
		return *this + (v - *this) *t;
	}

	float dot(const Vec4f &v) const {
		return x*v.x + y*v.y + z*v.z + w*v.w;
	}

	static void lerpArray(Vec4f *dest, const Vec4f *srcA, const Vec4f *srcB, float t, size_t size) {
		SSE2Vec4f::lerpArray(dest, srcA, srcB, t, size);
	}
};

inline Vec3f::Vec3f(const Vec4f &v): SSE2Vec4f(v) {}
#endif // USE_SSE2

}} //end namespace

#endif
