// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_XML_XMLPARSER_H_
#define _SHARED_XML_XMLPARSER_H_

#include <string>
#include <vector>

#include <xercesc/util/XercesDefs.hpp>
#include "vec.h"
#include "conversion.h"

using std::string;
using std::vector;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace XERCES_CPP_NAMESPACE {
	class DOMImplementation;
	class DOMDocument;
	class DOMNode;
	class DOMElement;
}

using XERCES_CPP_NAMESPACE::DOMImplementation;
//using XERCES_CPP_NAMESPACE::DOMDocument;
using XERCES_CPP_NAMESPACE::DOMNode;
using XERCES_CPP_NAMESPACE::DOMElement;

namespace Shared { namespace Xml {

const int strSize = 256;

class XmlIo;
class XmlTree;
class XmlNode;
class XmlAttribute;

// =====================================================
// 	class XmlIo
//
///	Wrapper for Xerces C++
// =====================================================

class XmlIo {
private:
	static bool initialized;
	DOMImplementation *implementation;

private:
	XmlIo();

public:
	static XmlIo &getInstance();
	~XmlIo();
	XmlNode *load(const string &path);
	void save(const string &path, const XmlNode *node);
	XmlNode *parseString(const char *doc, size_t size = (size_t)-1);
	/** WARNING: return value must be freed by calling XmlIo::getInstance().releaseString(). */
	char *toString(const XmlNode *node, bool pretty);
	void releaseString(char **domAllocatedString);
};

// =====================================================
//	class XmlTree
// =====================================================

class XmlTree{
private:
	XmlNode *rootNode;

private:
	XmlTree(XmlTree&);
	void operator =(XmlTree&);

public:
	XmlTree();
	~XmlTree();

	void init(const string &name);
	void load(const string &path);
	void save(const string &path);

	XmlNode *getRootNode() const	{return rootNode;}
};


// =====================================================
//	class XmlAttribute
// =====================================================

class XmlAttribute{
private:
	string name;
	string value;

private:
	XmlAttribute(XmlAttribute&);
	void operator =(XmlAttribute&);

public:
	XmlAttribute(DOMNode *attribute);
	XmlAttribute(const char *name, const char *value) : name(name), value(value){}
	XmlAttribute(const string &name, const string &value) : name(name), value(value){}

public:
	const string &getName() const						{return name;}
	const string &getValue() const						{return value;}

	bool getBoolValue() const;
	int getIntValue() const								{return strToInt(value);}
	int getIntValue(int min, int max) const;
	float getFloatValue() const							{return strToFloat(value);}
	float getFloatValue(float min, float max) const;
	const string &getRestrictedValue() const;
};

// =====================================================
//	class XmlNode
// =====================================================

class XmlNode{
private:
	string name;
	string text;
	vector<XmlNode*> children;
	vector<XmlAttribute*> attributes;

private:
	XmlNode(XmlNode&);
	void operator =(XmlNode&);

public:
	XmlNode(DOMNode *node);
	XmlNode(const string &name);
	~XmlNode();

	// get
	const string &getName() const	{return name;}
	int getChildCount() const		{return children.size();}
	int getAttributeCount() const	{return attributes.size();}
	const string &getText() const	{return text;}

	XmlAttribute *getAttribute(int i) const;
	XmlAttribute *getAttribute(const string &name, bool required = true) const;
	XmlNode *getChild(int i) const;
	XmlNode *getChild(const string &childName, int childIndex = 0, bool required = true) const;
	XmlNode *getParent() const;

	// get methods that return a specific type using the "value" attribute or appropriate attributes
	// for vector types
	const XmlAttribute *getValue() const 		{return getAttribute("value");}
	const string &getStringValue() const		{return getValue()->getValue();}
	const string &getRestrictedValue() const	{return getValue()->getRestrictedValue();}
	bool getBoolValue() const					{return getValue()->getBoolValue();}
	int getIntValue() const						{return getValue()->getIntValue();}
	float getFloatValue() const					{return getValue()->getFloatValue();}

	Vec2i getVec2iValue() const {
		return Vec2i(getAttribute("x")->getIntValue(),
				getAttribute("y")->getIntValue());
	}

	Vec3i getVec3iValue() const {
		return Vec3i(getAttribute("x")->getIntValue(),
				getAttribute("y")->getIntValue(),
				getAttribute("z")->getIntValue());
	}

	Vec4i getVec4iValue() const {
		return Vec4i(getAttribute("x")->getIntValue(),
				getAttribute("y")->getIntValue(),
				getAttribute("z")->getIntValue(),
				getAttribute("w")->getIntValue());
	}

	Vec2f getVec2fValue() const {
		return Vec2f(getFloatAttribute("x"),
				getFloatAttribute("y"));
	}

	Vec3f getVec3fValue() const {
		return Vec3f(getFloatAttribute("x"),
				getFloatAttribute("y"),
				getFloatAttribute("z"));
	}

	Vec4f getVec4fValue() const {
		return Vec4f(getFloatAttribute("x"),
				getFloatAttribute("y"),
				getFloatAttribute("z"),
				getFloatAttribute("w"));
	}

	Vec3f getColor3Value() const {
		return Vec3f(getFloatAttribute("red", 0.f, 1.0f),
				getFloatAttribute("green", 0.f, 1.0f),
				getFloatAttribute("blue", 0.f, 1.0f));
	}

	Vec4f getColor4Value() const {
		return Vec4f(getFloatAttribute("red", 0.f, 1.0f),
				getFloatAttribute("green", 0.f, 1.0f),
				getFloatAttribute("blue", 0.f, 1.0f),
				getFloatAttribute("alpha", 0.f, 1.0f));
	}


	// get methods that return a specific type using the "value" attribute or appropriate attributes
	// of the specified child node
	const XmlAttribute *getChildValue(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getAttribute("value");
	}

	const string &getChildStringValue(const string &childName, int childIndex = 0) const {
		return getChildValue(childName, childIndex)->getValue();
	}

	const string &getChildRestrictedValue(const string &childName, int childIndex = 0) const {
		return getChildValue(childName, childIndex)->getRestrictedValue();
	}

	bool getChildBoolValue(const string &childName, int childIndex = 0) const {
		return getChildValue(childName, childIndex)->getBoolValue();
	}

	int getChildIntValue(const string &childName, int childIndex = 0) const {
		return getChildValue(childName, childIndex)->getIntValue();
	}

	float getChildFloatValue(const string &childName, int childIndex = 0) const {
		return getChildValue(childName, childIndex)->getFloatValue();
	}

	Vec2i getChildVec2iValue(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getVec2iValue();
	}

	Vec3i getChildVec3iValue(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getVec3iValue();
	}

	Vec4i getChildVec4iValue(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getVec4iValue();
	}

	Vec2f getChildVec2fValue(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getVec2fValue();
	}

	Vec3f getChildVec3fValue(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getVec3fValue();
	}

	Vec4f getChildVec4fValue(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getVec4fValue();
	}

	Vec3f getChildColor3Value(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getColor3Value();
	}

	Vec4f getChildColor4Value(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getColor4Value();
	}

	// add
	XmlNode *addChild(const string &name);
	XmlAttribute *addAttribute(const string &name, int value)			{return addAttribute(name.c_str(), intToStr(value).c_str());}
	XmlAttribute *addAttribute(const string &name, float value)			{return addAttribute(name.c_str(), floatToStr(value).c_str());}
	XmlAttribute *addAttribute(const string &name, bool value)			{return addAttribute(name.c_str(), string(value ? "true" : "false").c_str());}
	XmlAttribute *addAttribute(const string &name, const char *value)	{return addAttribute(name.c_str(), value);}
	XmlAttribute *addAttribute(const string &name, const string &value)	{return addAttribute(name.c_str(), value.c_str());}
	XmlAttribute *addAttribute(const char *name, int value)				{return addAttribute(name, intToStr(value).c_str());}
	XmlAttribute *addAttribute(const char *name, float value)			{return addAttribute(name, floatToStr(value).c_str());}
	XmlAttribute *addAttribute(const char *name, bool value)			{return addAttribute(name, string(value ? "true" : "false").c_str());}
	XmlAttribute *addAttribute(const char *name, const char *value);
	XmlAttribute *addAttribute(const char *name, const string &value)	{return addAttribute(name, value.c_str());}

	// get
	int getIntAttribute(const string &childName) const							{return getAttribute(childName)->getIntValue();}
	int getIntAttribute(const string &childName, int min, int max) const		{return getAttribute(childName)->getIntValue(min, max);}
	float getFloatAttribute(const string &childName) const						{return getAttribute(childName)->getFloatValue();}
	float getFloatAttribute(const string &childName, float min, float max) const{return getAttribute(childName)->getFloatValue(min, max);}
	bool getBoolAttribute(const string &childName) const						{return getAttribute(childName)->getBoolValue();}
	const string &getStringAttribute(const string &childName) const				{return getAttribute(childName)->getValue();}
	const string &getRestrictedAttribute(const string &childName) const			{return getAttribute(childName)->getRestrictedValue();}

	XmlNode *addChild(const string &name, int value)			{return addChild(name, intToStr(value));}
	XmlNode *addChild(const string &name, float value)			{return addChild(name, floatToStr(value));}
	XmlNode *addChild(const string &name, bool value)			{return addChild(name, string(value ? "true" : "false"));}
	XmlNode *addChild(const string &name, const char *value)	{return addChild(name, string(value));}
	XmlNode *addChild(const string &name, const Vec2i &value)	{return addChild(name, intToStr(value.x), intToStr(value.y));}
	XmlNode *addChild(const string &name, const Vec3i &value)	{return addChild(name, intToStr(value.x), intToStr(value.y), intToStr(value.z));}
	XmlNode *addChild(const string &name, const Vec4i &value)	{return addChild(name, intToStr(value.x), intToStr(value.y), intToStr(value.z), intToStr(value.w));}
	XmlNode *addChild(const string &name, const Vec2f &value)	{return addChild(name, floatToStr(value.x), floatToStr(value.y));}
	XmlNode *addChild(const string &name, const Vec3f &value)	{return addChild(name, floatToStr(value.x), floatToStr(value.y), floatToStr(value.z));}
	XmlNode *addChild(const string &name, const Vec4f &value)	{return addChild(name, floatToStr(value.x), floatToStr(value.y), floatToStr(value.z), floatToStr(value.w));}
	XmlNode *addChild(const string &name, const string &value)	{
		XmlNode *child = addChild(name);
		child->addAttribute("value", value);
		return child;
	}

	XmlNode *addChild(const string &name, const string &x, const string &y) {
		XmlNode *child = addChild(name);
		child->addAttribute("x", x);
		child->addAttribute("y", y);
		return child;
	}

	XmlNode *addChild(const string &name, const string &x, const string &y, const string &z) {
		XmlNode *child = addChild(name);
		child->addAttribute("x", x);
		child->addAttribute("y", y);
		child->addAttribute("z", z);
		return child;
	}

	XmlNode *addChild(const string &name, const string &x, const string &y, const string &z, const string &w) {
		XmlNode *child = addChild(name);
		child->addAttribute("x", x);
		child->addAttribute("y", y);
		child->addAttribute("z", z);
		child->addAttribute("w", w);
		return child;
	}

	void populateElement(DOMElement *node, XERCES_CPP_NAMESPACE::DOMDocument *document) const;
	DOMElement *buildElement(XERCES_CPP_NAMESPACE::DOMDocument *document) const;

	int getOptionalIntValue(const char* name, int defaultValue = 0) const {
		const XmlNode *node = getChild(name, 0, false);
		return !node ? defaultValue : node->getAttribute("value")->getIntValue();
	}

	float getOptionalFloatValue(const char* name, float defaultValue = 0.f) const {
		const XmlNode *node = getChild(name, 0, false);
		return !node ? defaultValue : node->getAttribute("value")->getFloatValue();
	}

	bool getOptionalBoolValue(const char* name, bool defaultValue = false) const {
		const XmlNode *node = getChild(name, 0, false);
		return !node ? defaultValue : node->getAttribute("value")->getBoolValue();
	}

	string getOptionalRestrictedValue(const char* name, const char* defaultValue = "") const {
		const XmlNode *node = getChild(name, 0, false);
		return !node ? string(defaultValue) : node->getAttribute("value")->getRestrictedValue();
	}

	/** WARNING: return value must be freed by calling XmlIo::getInstance().releaseString(). */
	char *toString(bool pretty) const {
		return XmlIo::getInstance().toString(this, pretty);
	}

private:
	string getTreeString() const;
};


}}//end namespace

#endif
