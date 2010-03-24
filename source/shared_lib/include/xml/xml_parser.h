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
#include <memory>
#include <sstream>

#define TIXML_USE_STL
#include "tinyxml.h"
#include "vec.h"
#include "conversion.h"

using std::string;
using std::vector;
using std::stringstream;
using std::auto_ptr;	// This isn't as good as shared_ptr from tr1 or boost, but it's better
						// than what we've been doing with toString()
using namespace Shared::Math;
using namespace Shared::Util;

namespace Shared { namespace Xml {

const int strSize = 256;
extern const string defaultIndent;

class XmlIo;
class XmlTree;
class XmlNode;
class XmlAttribute;

// =====================================================
// 	class XmlIo
//
///	Wrapper for TinyXML
// =====================================================

class XmlIo {
private:
	static bool initialized;

private:
	XmlIo() {}
	~XmlIo() {}

public:
	static XmlIo &getInstance();
	XmlNode *load(const string &path);
	void save(const string &path, const XmlNode *node);
	XmlNode *parseString(const char *doc, size_t size = (size_t)-1);
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
	XmlAttribute(TiXmlAttribute *attribute);
	XmlAttribute(const char *name, const char *value) : name(name), value(value) {}
	XmlAttribute(const string &name, const string &value) : name(name), value(value) {}

public:
	const string &getName() const						{return name;}
	const string &getValue() const						{return value;}
	string toString() const								{return name + "=\"" + value + "\""; }
	void toString(stringstream &str) const				{str << name << "=\"" << value << "\"";}

	bool getBoolValue() const;
	int getIntValue() const								{return Conversion::strToInt(value);}
	int getIntValue(int min, int max) const;
	float getFloatValue() const							{return Conversion::strToFloat(value);}
	float getFloatValue(float min, float max) const;
	const string &getRestrictedValue() const;
};

// =====================================================
//	class XmlNode
// =====================================================

class XmlNode {
public:
	typedef vector<XmlNode*> Nodes;
	typedef vector<XmlAttribute*> Attributes;

private:
	string name;
	Nodes children;
	Attributes attributes;
	string text;

private:
	XmlNode(XmlNode&);
	void operator =(XmlNode&);

public:
	XmlNode(TiXmlNode *node);
	XmlNode(const string &name);
	~XmlNode();

	// get
	const string &getName() const	{return name;}
	int getChildCount() const		{return children.size();}
	int getAttributeCount() const	{return attributes.size();}

	XmlAttribute *getAttribute(int i) const;
	XmlAttribute *getAttribute(const string &name, bool required = true) const;
	XmlNode *getChild(int i) const;
	XmlNode *getOptionalChild(const string &childName) const;
	XmlNode *getChild(const string &childName, int childIndex = 0, bool required = true) const;

	XmlNode *getParent() const;
	const string &getText() const	{return text;}

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

	Vec3f getColor3Value() const;
	Vec4f getColor4Value() const;

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
	XmlAttribute *addAttribute(const string &name, int value)			{return addAttribute(name.c_str(), Conversion::toStr(value).c_str());}
	XmlAttribute *addAttribute(const string &name, float value)			{return addAttribute(name.c_str(), Conversion::toStr(value).c_str());}
	XmlAttribute *addAttribute(const string &name, bool value)			{return addAttribute(name.c_str(), string(value ? "true" : "false").c_str());}
	XmlAttribute *addAttribute(const string &name, const char *value)	{return addAttribute(name.c_str(), value);}
	XmlAttribute *addAttribute(const string &name, const string &value)	{return addAttribute(name.c_str(), value.c_str());}
	XmlAttribute *addAttribute(const char *name, int value)				{return addAttribute(name, Conversion::toStr(value).c_str());}
	XmlAttribute *addAttribute(const char *name, float value)			{return addAttribute(name, Conversion::toStr(value).c_str());}
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

	XmlNode *addChild(const string &name, int value)			{return addChild(name, Conversion::toStr(value));}
	XmlNode *addChild(const string &name, float value)			{return addChild(name, Conversion::toStr(value));}
	XmlNode *addChild(const string &name, bool value)			{return addChild(name, string(value ? "true" : "false"));}
	XmlNode *addChild(const string &name, const char *value)	{return addChild(name, string(value));}
	XmlNode *addChild(const string &name, const Vec2i &value)	{return addChild(name, Conversion::toStr(value.x), Conversion::toStr(value.y));}
	XmlNode *addChild(const string &name, const Vec3i &value)	{return addChild(name, Conversion::toStr(value.x), Conversion::toStr(value.y), Conversion::toStr(value.z));}
	XmlNode *addChild(const string &name, const Vec4i &value)	{return addChild(name, Conversion::toStr(value.x), Conversion::toStr(value.y), Conversion::toStr(value.z), Conversion::toStr(value.w));}
	XmlNode *addChild(const string &name, const Vec2f &value)	{return addChild(name, Conversion::toStr(value.x), Conversion::toStr(value.y));}
	XmlNode *addChild(const string &name, const Vec3f &value)	{return addChild(name, Conversion::toStr(value.x), Conversion::toStr(value.y), Conversion::toStr(value.z));}
	XmlNode *addChild(const string &name, const Vec4f &value)	{return addChild(name, Conversion::toStr(value.x), Conversion::toStr(value.y), Conversion::toStr(value.z), Conversion::toStr(value.w));}
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

	void populateElement(TiXmlElement *node) const;
	//DOMElement *buildElement(XERCES_CPP_NAMESPACE::DOMDocument *document) const;

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

	auto_ptr<string> toString(bool pretty = false, const string &indentSingle = defaultIndent) const;

	void toStringSimple(stringstream &str) const;
	void toStringPretty(stringstream &str, string &indent, const string &indentSingle) const;

private:
	string getTreeString() const;
};


// =====================================================
//	class XmlTree
// =====================================================

class XmlTree {
private:
	XmlNode *rootNode;

private:
	XmlTree(const XmlTree &);
	void operator =(const XmlTree &);

public:
	XmlTree() : rootNode(NULL)		{}
	XmlTree(const string &name) : rootNode(new XmlNode(name)) {}
	~XmlTree()						{delete rootNode;}

	void init(const string &name)	{rootNode = new XmlNode(name);}
	void load(const string &path)	{rootNode = XmlIo::getInstance().load(path);}
	void save(const string &path)	{XmlIo::getInstance().save(path, rootNode);}
	void parse(const string &xml)	{rootNode = XmlIo::getInstance().parseString(xml.c_str());}

	auto_ptr<string> toString(bool pretty = false, const string &indentSingle = defaultIndent) const {
		return rootNode->toString(pretty, indentSingle);
	}
	XmlNode *getRootNode() const	{return rootNode;}
};

}}//end namespace

#endif
