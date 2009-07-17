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

#ifndef _SHARED_UTIL_PROPERTIES_H_
#define _SHARED_UTIL_PROPERTIES_H_

#include <string>
#include <map>
#include <vector>
#include <cassert>

#include "conversion.h"

using std::map;
using std::vector;
using std::string;
using std::pair;
using std::less;
using std::range_error;

namespace Shared { namespace Util {

class Variant {
public:
	enum Type {
		typeString,
  		typeStringOffset,
		typeChar,
		typeUChar,
		typeShort,
		typeUShort,
		typeInt,
		typeUint,
		typeFloat,
		typeDouble,

		typeCount
	};

	unsigned char type;
	union {
		const char* str;
		size_t strOffset;
		char c;
		unsigned char uc;
		short s;
		unsigned short us;
		int i;
		unsigned int ui;
		float f;
		double d;
	} value;
	
public:
//	template < T> Variant(T value) : {setValue(value);}
	virtual ~Variant();

	void setValue(const char* value)		{type = typeString; this->value.str = value;}
	void setStrOffsetValue(size_t strOffset){type = typeStringOffset; this->value.strOffset = strOffset;}
	void setValue(char value)				{type = typeChar; this->value.c = value;}
	void setValue(unsigned char value)		{type = typeUChar; this->value.uc = value;}
	void setValue(short value)				{type = typeShort; this->value.s = value;}
	void setValue(unsigned short value)		{type = typeUShort; this->value.us = value;}
	void setValue(int value)				{type = typeInt; this->value.i = value;}
	void setValue(unsigned int value)		{type = typeUint; this->value.ui = value;}
	void setValue(float value)				{type = typeFloat; this->value.f = value;}
	void setValue(double value)				{type = typeDouble; this->value.d = value;}

	Type getType() const					{return (Type)type;}

	const char* getStrValue() const			{verify(typeString); return value.str;}
	const char* getStrValue(const char* base) const	{verify(typeStringOffset); return value.str;}
	char getCharValue() const				{verify(typeChar); return value.c;}
	unsigned char getUCharValue() const		{verify(typeUChar); return value.uc;}
	short getShortValue() const				{verify(typeShort); return value.s;}
	unsigned short getUShortValue() const	{verify(typeUShort); return value.us;}
	int getIntValue() const					{verify(typeInt); return value.i;}
	unsigned int getUintValue() const		{verify(typeUint); return value.ui;}
	float getFloatValue() const				{verify(typeFloat); return value.f;}
	double getDoubleValue() const			{verify(typeDouble); return value.d;}

private:
	void verify(Type type) const			{assert(this->type == type);}
};

template<class Key, class Compare = std::less<Key> > class VariantMap {
public:
	typedef map<Key, Variant, Compare> Map;

private:
	Map _map;
	const char *strings;

public:
	VariantMap();
	virtual ~VariantMap();

	const char* getStrValue(const Key &key) const		{return getValue(key).getStrValue();}
	char getCharValue(const Key &key) const				{return getValue(key).getCharValue();}
	unsigned char getUCharValue(const Key &key) const	{return getValue(key).getUCharValue();}
	short getShortValue(const Key &key) const			{return getValue(key).getShortValue();}
	unsigned short getUShortValue(const Key &key) const	{return getValue(key).getUShortValue();}
	int getIntValue(const Key &key) const				{return getValue(key).getIntValue();}
	unsigned int getUintValue(const Key &key) const		{return getValue(key).getUintValue();}
	float getFloatValue(const Key &key) const			{return getValue(key).getFloatValue();}
	double getDoubleValue(const Key &key) const			{return getValue(key).getDoubleValue();}
	
private:
	const Variant &getValue(const Key &key) const {
		Map it = _map.find(key);
		if (it == _map.end()) {
			throw range_error();
		} else {
			return it->second;
		}
	}
};

// =====================================================
// class Properties
//
/// ini-like file loader
// =====================================================

class Properties {
private:
	static const int maxLine = 8192;

public:
	typedef pair<string, string> PropertyPair;
	typedef map<string, string> PropertyMap;
	typedef vector<PropertyPair> PropertyVector;

private:
	PropertyVector propertyVector;
	PropertyMap propertyMap;
	string path;

public:
	Properties();
	void clear();
	void load(const string &path);
	void save(const string &path);

	const PropertyMap &getPropertyMap() const								{return propertyMap;}
	bool getBool(const string &key) const									{return _getBool(key);}
	bool getBool(const string &key, bool def) const							{return _getBool(key, &def);}
	int getInt(const string &key) const										{return _getInt(key);}
	int getInt(const string &key, int def) const							{return _getInt(key, &def);}
	int getInt(const string &key, int min, int max) const					{return _getInt(key, NULL, &min, &max);}
	int getInt(const string &key, int def, int min, int max) const			{return _getInt(key, &def, &min, &max);}
	float getFloat(const string &key) const									{return _getFloat(key);}
	float getFloat(const string &key, float def) const						{return _getFloat(key, &def);}
	float getFloat(const string &key, float min, float max) const			{return _getFloat(key, NULL, &min, &max);}
	float getFloat(const string &key, float def, float min, float max) const{return _getFloat(key, &def, &min, &max);}
	const string &getString(const string &key) const						{return _getString(key);}
	const string &getString(const string &key, const string &def) const		{return _getString(key, &def);}
	int getPropertyCount()	{return propertyVector.size();}

	void setInt(const string &key, int value)		{setString(key, intToStr(value));}
	void setBool(const string &key, bool value)		{setString(key, boolToStr(value));}
	void setFloat(const string &key, float value)	{setString(key, floatToStr(value));}
	void setString(const string &key, const string &value) {
		propertyMap.erase(key);
		propertyMap.insert(PropertyPair(key, value));
	}
	
	string toString() const;

private:
	bool _getBool(const string &key, const bool *pDef = NULL) const;
	int _getInt(const string &key, const int *pDef = NULL, const int *pMin = NULL, const int *pMax = NULL) const;
	float _getFloat(const string &key, const float *pDef = NULL, const float *pMin = NULL, const float *pMax = NULL) const;
	const string &_getString(const string &key, const string *pDef = NULL) const;
	const string *_getString(const string &key, bool required) const;
};

}}//end namespace

#endif
