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

//#include "pch.h"
#include "xml_parser.h"

#include <fstream>
#include <stdexcept>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>

#include "conversion.h"

#include "leak_dumper.h"

XERCES_CPP_NAMESPACE_USE

#if !defined(WIN32) || !defined(WIN64) // windows is a piece of shit
#	define DOMDocument XERCES_CPP_NAMESPACE::DOMDocument
#endif

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

#if XERCES_VERSION_MAJOR < 3
	DOMBuilder *parser = NULL;
#else
	DOMLSParser *parser = NULL;
#endif
	DOMDocument *document = NULL;
	XmlNode *rootNode = NULL;

	try{
		ErrorHandler errorHandler;
#if XERCES_VERSION_MAJOR < 3
		parser= (static_cast<DOMImplementationLS*>(implementation))->createDOMBuilder(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
		parser->setErrorHandler(&errorHandler);
		parser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
		parser->setFeature(XMLUni::fgDOMValidation, true);
#else
		parser = (static_cast<DOMImplementationLS*>(implementation))->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
    		DOMConfiguration  *config = parser->getDomConfig();
		config->setParameter(XMLUni::fgXercesSchemaFullChecking, true);
		config->setParameter(XMLUni::fgDOMValidate, true);
#endif
		document= parser->parseURI(path.c_str());

		if(!document){
			throw runtime_error("Can not parse URL: " + path);
		}

		rootNode = new XmlNode(document->getDocumentElement());
		parser->release();
		return rootNode;
	}
	catch(const DOMException &e){
		if(rootNode) {
			delete rootNode;
		}
		if(parser) {
			parser->release();
		}
		throw runtime_error("Exception while loading: " + path + ": " + XMLString::transcode(e.msg));
	}
}

XmlNode *XmlIo::parseString(const char *doc, size_t size) {
#if XERCES_VERSION_MAJOR < 3
	DOMBuilder *parser = NULL;
#else
	DOMLSParser *parser = NULL;
	DOMConfiguration  *config = NULL;
#endif
	DOMDocument *document = NULL;
	XmlNode *rootNode = NULL;

	if(size == (size_t)-1) {
		size = strlen(doc);
	}

	try{
		ErrorHandler errorHandler;
		MemBufInputSource memInput((const XMLByte*)doc, size, (const XMLCh*)L"bite me");
		Wrapper4InputSource wrapperSource(&memInput, false);
#if XERCES_VERSION_MAJOR < 3
		parser= (static_cast<DOMImplementationLS*>(implementation))->createDOMBuilder(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
		parser->setErrorHandler(&errorHandler);
		parser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
		parser->setFeature(XMLUni::fgDOMValidation, true);
		document = parser->parse(wrapperSource);
#else
		parser = (static_cast<DOMImplementationLS*>(implementation))->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
		config = parser->getDomConfig();
		config->setParameter(XMLUni::fgXercesSchemaFullChecking, true);
		config->setParameter(XMLUni::fgDOMValidate, true);
		document = parser->parse(static_cast<DOMLSInput*>(&wrapperSource));
#endif
		

		if(document == NULL){
			throw runtime_error("Failed to parse in-memory document");
		}

		rootNode= new XmlNode(document->getDocumentElement());
		parser->release();
		return rootNode;
	}
	catch(const DOMException &e){
		if(rootNode) {
			delete rootNode;
		}
		if(parser) {
			parser->release();
		}
		throw runtime_error(string("Exception while parsing in-memory document: ") + XMLString::transcode(e.msg));
	}
}

void XmlIo::save(const string &path, const XmlNode *node){
#if XERCES_VERSION_MAJOR < 3		
	DOMWriter* writer = NULL;
#else
	DOMLSSerializer *serializer = NULL;
	DOMLSOutput* output = NULL;
	DOMConfiguration* config = NULL;
#endif
	DOMDocument *document = NULL;
	DOMElement *documentElement = NULL;
	

	try{
		XMLCh str[strSize];
		XMLString::transcode(node->getName().c_str(), str, strSize-1);

		document = implementation->createDocument(0, str, 0);
		documentElement = document->getDocumentElement();
		
		node->populateElement(documentElement, document);

		LocalFileFormatTarget file(path.c_str());
#if XERCES_VERSION_MAJOR < 3		
		DOMWriter* writer = implementation->createDOMWriter();
		writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);
		writer->writeNode(&file, *document);
#else	
		serializer = implementation->createLSSerializer();
		output=implementation->createLSOutput();
		config=serializer->getDomConfig();
		config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint,true);
		output->setByteStream(&file);
		serializer->write(document,output);
		output->release();
		serializer->release();
#endif
		
		document->release();
	}
	catch(const DOMException &e){
#if XERCES_VERSION_MAJOR < 3
		if(writer) {
			writer->release();
		}
#else
		if(serializer) {
			serializer->release();
		}
		if(output) {
			output->release();
		}
#endif
		
		if(document) {
			document->release();
		}
		throw runtime_error("Exception while saving: " + path + ": " + XMLString::transcode(e.msg));
	}
}

/** WARNING: return value must be freed by calling XmlIo::getInstance().releaseString(). */
char *XmlIo::toString(const XmlNode *node, bool pretty) {
#if XERCES_VERSION_MAJOR < 3		
	DOMWriter* writer = NULL;
#else
	DOMLSSerializer *serializer = NULL;
	DOMLSOutput* output = NULL;
	DOMConfiguration* config = NULL;
#endif
	XMLCh str[strSize];
	DOMDocument *document = NULL;
	DOMElement *documentElement;
	XMLCh *xmlText = NULL;
	char *ret = NULL;

	try {
		XMLString::transcode(node->getName().c_str(), str, strSize-1);

		document = implementation->createDocument(0, str, 0);
		documentElement = document->getDocumentElement();

		node->populateElement(documentElement, document);

		// retrieve as string
#if XERCES_VERSION_MAJOR < 3
		DOMWriter* writer = implementation->createDOMWriter();
		writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, pretty);
		writer->setEncoding((const XMLCh*)L"UTF-8");
		xmlText = writer->writeToString(*document);
		ret = XMLString::transcode(xmlText);
		XMLString::release(&xmlText);
		writer->release();
#else
		serializer = implementation->createLSSerializer();
		output=implementation->createLSOutput();
		config=serializer->getDomConfig();
		config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint,true);
		//output->setByteStream(&file);
		xmlText = serializer->writeToString(document);
		ret = XMLString::transcode(xmlText);
		XMLString::release(&xmlText);
		output->release();
		serializer->release();
#endif
		
		document->release();
		return ret;
	}
	catch(const DOMException &e){
#if XERCES_VERSION_MAJOR < 3
		if(writer) {
			writer->release();
		}
#else
		if(serializer) {
			serializer->release();
		}
		if(output) {
			output->release();
		}
#endif
		if(xmlText) {
			XMLString::release(&xmlText);
		}
		if(document) {
			document->release();
		}
		throw runtime_error(string("Exception while converting to string: ") + XMLString::transcode(e.msg));
	}
}

void XmlIo::releaseString(char **domAllocatedString) {
	XMLString::release(domAllocatedString);
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

	//get value
	if(node->getNodeType()==DOMNode::ELEMENT_NODE && children.size()==0){
		char *textStr= XMLString::transcode(node->getTextContent());
		text= textStr;
		XMLString::release(&textStr);
	}
}

XmlNode::XmlNode(const string &name) {
	this->name = name;
}

XmlNode::~XmlNode(){
	for (int i = 0; i < children.size(); ++i) {
		delete children[i];
	}
	for (int i = 0; i < attributes.size(); ++i) {
		delete attributes[i];
	}
}

XmlAttribute *XmlNode::getAttribute(int i) const{
	if (i >= attributes.size()) {
		throw runtime_error(getName() + " node doesn't have " + intToStr(i) + " attributes");
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
	throw runtime_error("'" + getName() + "' node doesn't have an attribute named '" + name + "'");
}

XmlNode *XmlNode::getChild(int i) const {
	if (i >= children.size()) {
		throw runtime_error("'" + getName() + "' node doesn't have "
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
   if ( i == 0 )
      throw runtime_error("Node '" + getName() + "' doesn't have a"
			+ " child named  '" + childName + "'");
   else
	   throw runtime_error("Node '" + getName() + "' doesn't have "
		   + intToStr(i + 1) + " children named  '" + childName + "'");
}

XmlNode *XmlNode::addChild(const string &name){
	XmlNode *node= new XmlNode(name);
	children.push_back(node);
	return node;
}

XmlAttribute *XmlNode::addAttribute(const char *name, const char *value){
	XmlAttribute *attr= new XmlAttribute(name, value);
	attributes.push_back(attr);
	return attr;
}

void XmlNode::populateElement(DOMElement *node, DOMDocument *document) const {
	XMLCh str[strSize];
	for (int i = 0; i < attributes.size(); ++i) {
		XMLString::transcode(attributes[i]->getName().c_str(), str, strSize - 1);
		DOMAttr *attr = document->createAttribute(str);

		XMLString::transcode(attributes[i]->getValue().c_str(), str, strSize - 1);
		attr->setValue(str);

		node->setAttributeNode(attr);
	}

	for (int i = 0; i < children.size(); ++i) {
		node->appendChild(children[i]->buildElement(document));
	}
}

DOMElement *XmlNode::buildElement(DOMDocument *document) const {
	XMLCh str[strSize];
	XMLString::transcode(name.c_str(), str, strSize - 1);
	DOMElement *node = document->createElement(str);

	populateElement(node, document);
	return node;
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

bool XmlAttribute::getBoolValue() const {
	if(value == "true") {
		return true;
	} else if (value == "false") {
		return false;
	} else {
		throw runtime_error("Not a valid bool value (true or false): " + getName() + ": " + value);
	}
}


int XmlAttribute::getIntValue(int min, int max) const{
	int i= strToInt(value);
	if(i<min || i>max){
		throw runtime_error("Xml Attribute int out of range: " + getName() + ": " + value);
	}
	return i;
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
