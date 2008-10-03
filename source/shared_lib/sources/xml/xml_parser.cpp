// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "xml_parser.h"

#include <fstream>
#include <stdexcept>

#include "conversion.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

#include "leak_dumper.h"


XERCES_CPP_NAMESPACE_USE

using namespace std;

namespace Shared{ namespace Xml{

using namespace Util;

// =====================================================
//	class ErrorHandler
// =====================================================

class ErrorHandler: public DOMErrorHandler{
public:
	virtual bool handleError (const DOMError &domError){
		if(domError.getSeverity()== DOMError::DOM_SEVERITY_FATAL_ERROR){
			char msgStr[strSize], fileStr[strSize];
			XMLString::transcode(domError.getMessage(), msgStr, strSize-1);
			XMLString::transcode(domError.getLocation()->getURI(), fileStr, strSize-1);
			int lineNumber= domError.getLocation()->getLineNumber();
			throw runtime_error("Error parsing XML, file: " + string(fileStr) + ", line: " + intToStr(lineNumber) + ": " + string(msgStr));
		}
		return true;
	}
};

// =====================================================
//	class XmlIo
// =====================================================

bool XmlIo::initialized= false;

XmlIo::XmlIo(){
	try{
		XMLPlatformUtils::Initialize();
	}
	catch(const XMLException&){
		throw runtime_error("Error initializing XML system");
	}

	try{
        XMLCh str[strSize];
        XMLString::transcode("LS", str, strSize-1);

		implementation = DOMImplementationRegistry::getDOMImplementation(str);
	}
	catch(const DOMException){
		throw runtime_error("Exception while creating XML parser");
	}
}

XmlIo &XmlIo::getInstance(){
	static XmlIo XmlIo;
	return XmlIo;
}

XmlIo::~XmlIo(){
	XMLPlatformUtils::Terminate();
}

XmlNode *XmlIo::load(const string &path){

	try{
		ErrorHandler errorHandler;
		DOMBuilder *parser= (static_cast<DOMImplementationLS*>(implementation))->createDOMBuilder(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
		parser->setErrorHandler(&errorHandler);
		parser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
		parser->setFeature(XMLUni::fgDOMValidation, true);
		DOMDocument *document= parser->parseURI(path.c_str());

		if(document==NULL){
			throw runtime_error("Can not parse URL: " + path);
		}

		XmlNode *rootNode= new XmlNode(document->getDocumentElement());
		parser->release();
		return rootNode;
	}
	catch(const DOMException &e){
		throw runtime_error("Exception while loading: " + path + ": " + XMLString::transcode(e.msg));
	}
}

void XmlIo::save(const string &path, const XmlNode *node){
	try{
		XMLCh str[strSize];
		XMLString::transcode(node->getName().c_str(), str, strSize-1);

		DOMDocument *document= implementation->createDocument(0, str, 0);
		DOMElement *documentElement= document->getDocumentElement();

		for(int i=0; i<node->getChildCount(); ++i){
			documentElement->appendChild(node->getChild(i)->buildElement(document));
		}

		LocalFileFormatTarget file(path.c_str());
		DOMWriter* writer = implementation->createDOMWriter();
		writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
		writer->writeNode(&file, *document);
		document->release();
	}
	catch(const DOMException &e){
		throw runtime_error("Exception while saving: " + path + ": " + XMLString::transcode(e.msg));
	}
}
// =====================================================
//	class XmlTree
// =====================================================

XmlTree::XmlTree(){
	rootNode= NULL;
}

void XmlTree::init(const string &name){
	this->rootNode= new XmlNode(name);
}

void XmlTree::load(const string &path){
	this->rootNode= XmlIo::getInstance().load(path);
}

void XmlTree::save(const string &path){
	XmlIo::getInstance().save(path, rootNode);
}

XmlTree::~XmlTree(){
	delete rootNode;
}

// =====================================================
//	class XmlNode
// =====================================================

XmlNode::XmlNode(DOMNode *node){

	//get name
	char str[strSize];
	XMLString::transcode(node->getNodeName(), str, strSize-1);
	name= str;

	//check document
	if(node->getNodeType()==DOMNode::DOCUMENT_NODE){
		name="document";
	}

	//check children
	for(int i=0; i<node->getChildNodes()->getLength(); ++i){
		DOMNode *currentNode= node->getChildNodes()->item(i);
		if(currentNode->getNodeType()==DOMNode::ELEMENT_NODE){
			XmlNode *xmlNode= new XmlNode(currentNode);
			children.push_back(xmlNode);
		}
	}

	//check attributes
	DOMNamedNodeMap *domAttributes= node->getAttributes();
	if(domAttributes!=NULL){
		for(int i=0; i<domAttributes->getLength(); ++i){
			DOMNode *currentNode= domAttributes->item(i);
			if(currentNode->getNodeType()==DOMNode::ATTRIBUTE_NODE){
				XmlAttribute *xmlAttribute= new XmlAttribute(domAttributes->item(i));
				attributes.push_back(xmlAttribute);
			}
		}
	}
}

XmlNode::XmlNode(const string &name){
	this->name= name;
}

XmlNode::~XmlNode(){
	for(int i=0; i<children.size(); ++i){
		delete children[i];
	}
	for(int i=0; i<attributes.size(); ++i){
		delete attributes[i];
	}
}

XmlAttribute *XmlNode::getAttribute(int i) const{
	if(i>=attributes.size()){
		throw runtime_error(getName()+" node doesn't have "+intToStr(i)+" attributes");
	}
	return attributes[i];
}

XmlAttribute *XmlNode::getAttribute(const string &name, bool required) const{
	for (int i = 0; i < attributes.size(); ++i) {
		if (attributes[i]->getName() == name) {
			return attributes[i];
		}
	}
	if (!required) {
		return NULL;
	}
	throw runtime_error("\"" + getName() + "\" node doesn't have a attribute named \"" + name + "\"");
}

XmlNode *XmlNode::getChild(int i) const {
	if (i >= children.size()) {
		throw runtime_error("\"" + getName() + "\" node doesn't have "
				+ intToStr(i + 1) + " children");
	}
	return children[i];
}

XmlNode *XmlNode::getChild(const string &childName, int i, bool required) const{
	int count = 0;
	if (i < children.size()) {
		for (int j = 0; j < children.size(); ++j) {
			if (children[j]->getName() == childName) {
				if (count == i) {
					return children[j];
				}
				count++;
			}
		}
	}

	if (!required) {
		return NULL;
	}
	throw runtime_error("Node \"" + getName() + "\" doesn't have "
			+ intToStr(i + 1) + " children named  \"" + childName
			+ "\"\n\nTree: " + getTreeString());
}

XmlNode *XmlNode::addChild(const string &name){
	XmlNode *node= new XmlNode(name);
	children.push_back(node);
	return node;
}

XmlAttribute *XmlNode::addAttribute(const string &name, const string &value){
	XmlAttribute *attr= new XmlAttribute(name, value);
	attributes.push_back(attr);
	return attr;
}

DOMElement *XmlNode::buildElement(DOMDocument *document) const{
	XMLCh str[strSize];
	XMLString::transcode(name.c_str(), str, strSize-1);

	DOMElement *node= document->createElement(str);

	for(int i=0; i<attributes.size(); ++i){
        XMLString::transcode(attributes[i]->getName().c_str(), str, strSize-1);
		DOMAttr *attr= document->createAttribute(str);

		XMLString::transcode(attributes[i]->getValue().c_str(), str, strSize-1);
		attr->setValue(str);

		node->setAttributeNode(attr);
	}

	for(int i=0; i<children.size(); ++i){
		node->appendChild(children[i]->buildElement(document));
	}

	return node;
}

int XmlNode::getOptionalIntValue(const char* name, int defaultValue) const {
	const XmlNode *node = getChild(name, 0, false);
	return !node ? defaultValue : node->getAttribute("value")->getIntValue();
}

float XmlNode::getOptionalFloatValue(const char* name, float defaultValue) const {
	const XmlNode *node = getChild(name, 0, false);
	return !node ? defaultValue : node->getAttribute("value")->getFloatValue();
}

string XmlNode::getOptionalRestrictedValue(const char* name, const char* defaultValue) const {
	const XmlNode *node = getChild(name, 0, false);
	return !node ? string(defaultValue) : node->getAttribute("value")->getRestrictedValue();
}

string XmlNode::getTreeString() const{
	string str;

	str+= getName();

	if(!children.empty()){
		str+= " (";
		for(int i=0; i<children.size(); ++i){
			str+= children[i]->getTreeString();
			str+= " ";
		}
		str+=") ";
	}

	return str;
}

// =====================================================
//	class XmlAttribute
// =====================================================

XmlAttribute::XmlAttribute(DOMNode *attribute){
	char str[strSize];

	XMLString::transcode(attribute->getNodeValue(), str, strSize-1);
	value= str;

	XMLString::transcode(attribute->getNodeName(), str, strSize-1);
	name= str;
}

XmlAttribute::XmlAttribute(const string &name, const string &value){
	this->name= name;
	this->value= value;
}

bool XmlAttribute::getBoolValue() const{
	if(value=="true"){
		return true;
	}
	else if(value=="false"){
		return false;
	}
	else{
		throw runtime_error("Not a valid bool value (true or false): " +getName()+": "+ value);
	}
}

int XmlAttribute::getIntValue() const{
	return strToInt(value);
}

int XmlAttribute::getIntValue(int min, int max) const{
	int i= strToInt(value);
	if(i<min || i>max){
		throw runtime_error("Xml Attribute int out of range: " + getName() + ": " + value);
	}
	return i;
}

float XmlAttribute::getFloatValue() const{
	return strToFloat(value);
}

float XmlAttribute::getFloatValue(float min, float max) const{
	float f= strToFloat(value);
	if(f<min || f>max){
		throw runtime_error("Xml attribute float out of range: " + getName() + ": " + value);
	}
	return f;
}

const string &XmlAttribute::getRestrictedValue() const
{
	const string allowedCharacters = "abcdefghijklmnopqrstuvwxyz1234567890._-/";

	for(int i= 0; i<value.size(); ++i){
		if(allowedCharacters.find(value[i])==string::npos){
			throw runtime_error(
				string("The string \"" + value + "\" contains a character that is not allowed: \"") + value[i] +
				"\"\nFor portability reasons the only allowed characters in this field are: " + allowedCharacters);
		}
	}

	return value;
}

}}//end namespace
