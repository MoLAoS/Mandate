#include "configuration.h"

#include <stdexcept>

#include "xml/xml_parser.h"
#include "util/util.h"
#include "util/properties.h"
#include "util/conversion.h"

using namespace std;
using namespace Shared::Xml;
using namespace Shared::Util;

namespace Configurator {

// ===============================================
// 	class Configuration
// ===============================================

Configuration::~Configuration() {
	for (int i = 0; i < fieldGroups.size(); ++i) {
		delete fieldGroups[i];
	}
}

void Configuration::load(const string &path) {
	loadStructure(path);
	loadValues(fileName);
}

void Configuration::loadStructure(const string &path) {
	XmlTree xmlTree;
	xmlTree.load(path);

	const XmlNode *configurationNode = xmlTree.getRootNode();

	//title
	title = configurationNode->getChild("title")->getAttribute("value")->getValue();

	//fileName
	fileName = configurationNode->getChild("file-name")->getAttribute("value")->getValue();

	//icon
	XmlNode *iconNode = configurationNode->getChild("icon");
	icon = iconNode->getAttribute("value")->getBoolValue();

	if (icon) {
		iconPath = iconNode->getAttribute("path")->getValue();
	}

	try {
		//boolMode
		XmlNode *boolModeNode = configurationNode->getChild("bool-mode");
		this->intBool = (boolModeNode->getAttribute("value")->getValue() == "int");
	}
	catch (const exception &) {
		this->intBool = false;
	}

	const XmlNode *fieldGroupsNode = configurationNode->getChild("field-groups");

	fieldGroups.resize(fieldGroupsNode->getChildCount());

	for (int i = 0; i < fieldGroups.size(); ++i) {
		const XmlNode *fieldGroupNode = fieldGroupsNode->getChild("field-group", i);
		FieldGroup *fieldGroup = new FieldGroup();
		fieldGroup->load(fieldGroupNode);
		fieldGroups[i] = fieldGroup;
	}
}

void Configuration::loadValues(const string &path) {
	Properties properties;

	properties.load(path);

	for (int fgI = 0; fgI < fieldGroups.size(); ++fgI) {
		FieldGroup *fg = fieldGroups[fgI];

		for (int fI = 0; fI < fg->getFieldCount(); ++fI) {
			Field *f = fg->getField(fI);
			f->setValue(properties.getString(f->getVariableName()));
		}
	}
}

void Configuration::save() {
	Properties properties;

	properties.load(fileName);

	for (int fgI = 0; fgI < fieldGroups.size(); ++fgI) {
		FieldGroup *fg = fieldGroups[fgI];

		for (int fI = 0; fI < fg->getFieldCount(); ++fI) {
			Field *f = fg->getField(fI);
			f->updateValue();

			if (!f->isValueValid(f->getValue())) {
				f->setValue(f->getDefaultValue());
				f->updateControl();
			}

			properties.setString(f->getVariableName(), f->getValue());
		}
	}

	properties.save(fileName);
}

string Field::getInfo() const {
	return name + " (default: " + defaultValue + ")";
}

// ===============================================
// 	class FieldGroup
// ===============================================

FieldGroup::~FieldGroup() {
	for (int i = 0; i < fields.size(); ++i) {
		delete fields[i];
	}
}

void FieldGroup::load(const XmlNode *groupNode) {

	name = groupNode->getAttribute("name")->getValue();

	fields.resize(groupNode->getChildCount());

	for (int i = 0; i < fields.size(); ++i) {
		const XmlNode *fieldNode = groupNode->getChild("field", i); 

		Field *f = newField(fieldNode->getAttribute("type")->getValue());
			
		//name
		const XmlNode *nameNode = fieldNode->getChild("name");
		f->setName(nameNode->getAttribute("value")->getValue());

		//variableName
		const XmlNode *variableNameNode = fieldNode->getChild("variable-name");
		f->setVariableName(variableNameNode->getAttribute("value")->getValue());

		//description
		const XmlNode *descriptionNode = fieldNode->getChild("description");
		f->setDescription(descriptionNode->getAttribute("value")->getValue());

		//default
		const XmlNode *defaultNode = fieldNode->getChild("default");
		f->setDefaultValue(defaultNode->getAttribute("value")->getValue());

		f->loadSpecific(fieldNode);

		if (!f->isValueValid(f->getDefaultValue())) {
			throw runtime_error("Default value not valid in field: " + f->getName());
		}

		fields[i] = f;
	}
}

Field *FieldGroup::newField(const string &type) {
	if (type == "Bool") {
		return new BoolField();
	}
	else if (type == "Int") {
		return new IntField();
	}
	else if (type == "Float") {
		return new FloatField();
	}
	else if (type == "String") {
		return new StringField();
	}
	else if (type == "Enum") {
		return new EnumField();
	}
	else if (type == "IntRange") {
		return new IntRangeField();
	}
	else if (type == "FloatRange") {
		return new FloatRangeField();
	}
	else {
		throw runtime_error("Unknown field type: " + type);
	}
}

// ===============================================
// 	class BoolField
// ===============================================

void BoolField::createControl(wxWindow *parent, wxSizer *sizer) {
	checkBox = new wxCheckBox(parent, -1, wxT(""));
	checkBox->SetValue(Conversion::strToBool(value));
	sizer->Add(checkBox, 0, wxGROW);
}

void BoolField::updateValue() {
	value = Conversion::toStr(checkBox->GetValue());
}

void BoolField::updateControl() {
	checkBox->SetValue(Conversion::strToBool(value));
}

bool BoolField::isValueValid(const string &value) {
	try {
		Conversion::strToBool(value);
	}
	catch (const exception &) {
		return false;
	}

	return true;
}

// ===============================================
// 	class IntField
// ===============================================

void IntField::createControl(wxWindow *parent, wxSizer *sizer) {
	textCtrl = new wxTextCtrl(parent, -1, STRCONV(value.c_str()));
	sizer->Add(textCtrl, 0, wxGROW);
}

void IntField::updateValue() {
	value = textCtrl->GetValue().mb_str(wxConvUTF8);
}

void IntField::updateControl() {
	textCtrl->SetValue(STRCONV(value.c_str()));
}

bool IntField::isValueValid(const string &value) {
	try {
		Conversion::strToInt(value);
	}
	catch (const exception &) {
		return false;
	}

	return true;
}

// ===============================================
// 	class FloatField
// ===============================================

void FloatField::createControl(wxWindow *parent, wxSizer *sizer) {
	textCtrl = new wxTextCtrl(parent, -1, STRCONV(value.c_str()));
	sizer->Add(textCtrl, 0, wxGROW);
}

void FloatField::updateValue() {
	value = textCtrl->GetValue().mb_str(wxConvUTF8);
}

void FloatField::updateControl() {
	textCtrl->SetValue(STRCONV(value.c_str()));
}

bool FloatField::isValueValid(const string &value) {
	try {
		Conversion::strToFloat(value);
	}
	catch (const exception &) {
		return false;
	}
	return true;
}

// ===============================================
// 	class StringField
// ===============================================

void StringField::createControl(wxWindow *parent, wxSizer *sizer) {
	textCtrl = new wxTextCtrl(parent, -1, STRCONV(value.c_str()));
	textCtrl->SetSize(wxSize(3*textCtrl->GetSize().x / 2, textCtrl->GetSize().y));
	sizer->Add(textCtrl, 0, wxGROW);
}

void StringField::updateValue() {
	value = textCtrl->GetValue().mb_str(wxConvUTF8);
}

void StringField::updateControl() {
	textCtrl->SetValue(STRCONV(value.c_str()));
}

bool StringField::isValueValid(const string &value) {
	return true;
}

// ===============================================
// 	class EnumField
// ===============================================

void EnumField::createControl(wxWindow *parent, wxSizer *sizer) {
	comboBox = new wxComboBox(parent, -1, wxT(""), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);

	for (int i = 0; i < enumerants.size(); ++i) {
		comboBox->Append(STRCONV(enumerants[i].c_str()));
	}

	comboBox->SetValue(STRCONV(value.c_str()));
	sizer->Add(comboBox, 0, wxGROW);
}

void EnumField::updateValue() {
	value = comboBox->GetValue().mb_str(wxConvUTF8);
}

void EnumField::updateControl() {
	comboBox->SetValue(STRCONV(value.c_str()));
}

bool EnumField::isValueValid(const string &value) {
	return true;
}

void EnumField::loadSpecific(const XmlNode *fieldNode) {
	const XmlNode *enumsNode = fieldNode->getChild("enums");

	for (int i=0; i<enumsNode->getChildCount(); ++i) {
		const XmlNode *enumNode = enumsNode->getChild("enum", i);
		enumerants.push_back(enumNode->getAttribute("value")->getValue());
	}
};

// ===============================================
// 	class IntRange
// ===============================================

void IntRangeField::createControl(wxWindow *parent, wxSizer *sizer) {
	slider = new wxSlider(parent, -1, Conversion::strToInt(value), min, max, wxDefaultPosition, wxDefaultSize, wxSL_LABELS);
	sizer->Add(slider, 0, wxGROW);
}

void IntRangeField::updateValue() {
	value = Conversion::toStr(slider->GetValue());
}

void IntRangeField::updateControl() {
	slider->SetValue(Conversion::strToInt(value));
}

bool IntRangeField::isValueValid(const string &value) {
	try {
		Conversion::strToInt(value);
	}
	catch (const exception &) {
		return false;
	}

	return true;
}

void IntRangeField::loadSpecific(const XmlNode *fieldNode) {
	const XmlNode *minNode = fieldNode->getChild("min");
	min = Conversion::strToInt(minNode->getAttribute("value")->getValue());

	const XmlNode *maxNode = fieldNode->getChild("max");
	max = Conversion::strToInt(maxNode->getAttribute("value")->getValue());
}

string IntRangeField::getInfo() const {
	return name + " (min: " + intToStr(min)+ ", max: " + intToStr(max) + ", default: " + defaultValue + ")";
}

// ===============================================
// 	class FloatRangeField
// ===============================================

void FloatRangeField::createControl(wxWindow *parent, wxSizer *sizer) {
	textCtrl = new wxTextCtrl(parent, -1, STRCONV(value.c_str()));
	sizer->Add(textCtrl, 0, wxGROW);
}

void FloatRangeField::updateValue() {
	value = textCtrl->GetValue().mb_str(wxConvUTF8);
}

void FloatRangeField::updateControl() {
	textCtrl->SetValue(STRCONV(value.c_str()));
}

bool FloatRangeField::isValueValid(const string &value) {
	try {
		float f = Conversion::strToFloat(value);
		return f >= min && f <= max;
	} catch (const exception &) {
		return false;
	}

	return true;
}

void FloatRangeField::loadSpecific(const XmlNode *fieldNode) {
	const XmlNode *minNode = fieldNode->getChild("min");
	string str = minNode->getAttribute("value")->getValue();
	min = Conversion::strToFloat(str);

	const XmlNode *maxNode = fieldNode->getChild("max");
	max = Conversion::strToFloat(maxNode->getAttribute("value")->getValue());
};

string FloatRangeField::getInfo() const {
	stringstream str;
	str << name << " (min: " << min << ", max: " << max << ", default: " << defaultValue << ")";
	return str.str();
}

}//end namespace
