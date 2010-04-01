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
#include <iostream>
#include "simd.h"

using std::ostream;
using std::istream;

namespace Shared { namespace Math {

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

	Vec2<T>& operator *=(const T &v) {
		x *= v;
		y *= v;
		return *this;
	}

	Vec2<T>& operator /=(const T &v) {
		x /= v;
		y /= v;
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
	union {
		struct { T x, y, z; };
		struct { T r, g, b; };
		T raw[3];
	};

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
	union {
		struct { T x, y, z, w; };
		struct { T r, g, b, a; };
		T raw[4];
	};

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
typedef Vec3<float> Vec3f;
typedef Vec4<float> Vec4f;

typedef Vec2<double> Vec2d;
typedef Vec3<double> Vec3d;
typedef Vec4<double> Vec4d;

template <typename T> inline ostream& operator<<(ostream &stream, const Vec2<T> &vec) {
	return stream << "(" << vec.x << ", " << vec.y << ")";
}

template <typename T> inline istream& operator>>(istream &stream, Vec2<T> &vec) {
	char junk;
	return stream >> junk >> vec.x >> junk >> vec.y >> junk;
}

template <typename T> inline ostream& operator<<(ostream &stream, const Vec3<T> &vec) {
	return stream << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
}

template <typename T> inline istream& operator>>(istream &stream, Vec3<T> &vec) {
	char junk;
	return stream >> junk >> vec.x >> junk >> vec.y >> junk >> vec.z >> junk;
}

template <typename T> inline ostream& operator<<(ostream &stream, const Vec4<T> &vec) {
	return stream << "(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
}

template <typename T> inline istream& operator>>(istream &stream, Vec4<T> &vec) {
	char junk;
	return stream >> junk >> vec.x >> junk >> vec.y >> junk >> vec.z >> junk >> vec.w >> junk;
}

}} //end namespace

#endif
