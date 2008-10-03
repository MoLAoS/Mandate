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

namespace XERCES_CPP_NAMESPACE{
	class DOMImplementation;
	class DOMDocument;
	class DOMNode;
	class DOMElement;
}

namespace Shared{ namespace Xml{

const int strSize= 256;

class XmlIo;
class XmlTree;
class XmlNode;
class XmlAttribute;

// =====================================================
// 	class XmlIo
//
///	Wrapper for Xerces C++
// =====================================================

// =====================================================
//	class XmlAttribute
// =====================================================

class XmlAttribute{
private:
	string value;
	string name;

private:
	XmlAttribute(XmlAttribute&);
	void operator =(XmlAttribute&);

public:
	XmlAttribute(XERCES_CPP_NAMESPACE::DOMNode *attribute);
	XmlAttribute(const string &name, const string &value);

public:
	const string &getName() const	{return name;}
	const string &getValue() const	{return value;}

	bool getBoolValue() const;
	int getIntValue() const;
	int getIntValue(int min, int max) const;
	float getFloatValue() const;
	float getFloatValue(float min, float max) const;
	const string &getRestrictedValue() const;
};

// =====================================================
//	class XmlNode
// =====================================================

class XmlNode{
private:
	string name;
	vector<XmlNode*> children;
	vector<XmlAttribute*> attributes;

private:
	XmlNode(XmlNode&);
	void operator =(XmlNode&);

public:
	XmlNode(XERCES_CPP_NAMESPACE::DOMNode *node);
	XmlNode(const string &name);
	~XmlNode();

	// get
	const string &getName() const	{return name;}
	int getChildCount() const		{return children.size();}
	int getAttributeCount() const	{return attributes.size();}

	XmlAttribute *getAttribute(int i) const;
	XmlAttribute *getAttribute(const string &name, bool required = true) const;
	XmlNode *getChild(int i) const;
	XmlNode *getChild(const string &childName, int childIndex = 0, bool required = true) const;
	XmlNode *getParent() const;

	const XmlAttribute *getChildValue(const string &childName, int childIndex = 0) const {
		return getChild(childName, childIndex)->getAttribute("value");
	}

	const string &getChildStringValue(const string &childName, int childIndex = 0) const {
		return getChildValue(childName, childIndex)->getValue();
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
		XmlNode *child = getChild(childName, childIndex);
		return Vec2i(child->getAttribute("x")->getIntValue(), child->getAttribute("y")->getIntValue());
	}

	Vec3i getChildVec3iValue(const string &childName, int childIndex = 0) const {
		XmlNode *child = getChild(childName, childIndex);
		return Vec3i(child->getAttribute("x")->getIntValue(), child->getAttribute("y")->getIntValue(), child->getAttribute("z")->getIntValue());
	}

	Vec2f getChildVec2fValue(const string &childName, int childIndex = 0) const {
		XmlNode *child = getChild(childName, childIndex);
		return Vec2f(child->getAttribute("x")->getFloatValue(), child->getAttribute("y")->getFloatValue());
	}

	Vec3f getChildVec3fValue(const string &childName, int childIndex = 0) const {
		XmlNode *child = getChild(childName, childIndex);
		return Vec3f(child->getAttribute("x")->getFloatValue(), child->getAttribute("y")->getFloatValue(), child->getAttribute("z")->getFloatValue());
	}

	// add
	XmlNode *addChild(const string &name);
	XmlAttribute *addAttribute(const string &name, int value)			{return addAttribute(name, intToStr(value));}
	XmlAttribute *addAttribute(const string &name, float value)			{return addAttribute(name, floatToStr(value));}
	XmlAttribute *addAttribute(const string &name, bool value)			{return addAttribute(name, string(value ? "true" : "false"));}
	XmlAttribute *addAttribute(const string &name, const string &value);

	XmlNode *addChild(const string &name, int value)			{return addChild(name, intToStr(value));}
	XmlNode *addChild(const string &name, float value)			{return addChild(name, floatToStr(value));}
	XmlNode *addChild(const string &name, bool value)			{return addChild(name, string(value ? "true" : "false"));}
	XmlNode *addChild(const string &name, const Vec2i &value)	{return addChild(name, intToStr(value.x), intToStr(value.y));}
	XmlNode *addChild(const string &name, const Vec3i &value)	{return addChild(name, intToStr(value.x), intToStr(value.y), intToStr(value.z));}
	XmlNode *addChild(const string &name, const Vec2f &value)	{return addChild(name, floatToStr(value.x), floatToStr(value.y));}
	XmlNode *addChild(const string &name, const Vec3f &value)	{return addChild(name, floatToStr(value.x), floatToStr(value.y), floatToStr(value.z));}

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

	XERCES_CPP_NAMESPACE::DOMElement *buildElement(XERCES_CPP_NAMESPACE::DOMDocument *document) const;

	int getOptionalIntValue(const char* name, int defaultValue = 0) const;
	float getOptionalFloatValue(const char* name, float defaultValue = 1.0f) const;
	string getOptionalRestrictedValue(const char* name, const char* defaultValue) const;

private:
	string getTreeString() const;
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


class XmlIo{
private:
	static bool initialized;
	XERCES_CPP_NAMESPACE::DOMImplementation *implementation;

private:
	XmlIo();

public:
	static XmlIo &getInstance();
	~XmlIo();
	XmlNode *load(const string &path);
	void save(const string &path, const XmlNode *node);
};

}}//end namespace

#endif
