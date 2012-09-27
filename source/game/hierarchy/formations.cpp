// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#include "pch.h"
#include "formations.h"
#include "lang.h"
#include "logger.h"

namespace Glest { namespace Hierarchy {
using namespace ProtoTypes;
using namespace Global;

// =====================================================
// 	class Formation
// =====================================================

bool Formation::load(const XmlNode *formationNode, const string &dir) {
	bool loadOk = true;
	try { title = formationNode->getAttribute("title")->getRestrictedValue(); }
	catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
    if (!DisplayableType::load(formationNode, dir)) {
        loadOk = false;
	}
	try {
	    const XmlNode *linesNode = formationNode->getChild("lines");
	    lines.resize(linesNode->getChildCount());
        for (int i = 0; i < linesNode->getChildCount(); ++i) {
            const XmlNode* lineNode = linesNode->getChild("line", i);
            lines[i].name = lineNode->getAttribute("name")->getRestrictedValue();
            lines[i].line.resize(lineNode->getChildCount());
            for (int j = 0; j < lineNode->getChildCount(); ++j) {
                const XmlNode *offsetNode = lineNode->getChild("offset", j);
                lines[i].line[j].x = offsetNode->getChildIntValue("x");
                lines[i].line[j].y = offsetNode->getChildIntValue("y");
            }
        }
	} catch (runtime_error e) {
		g_logger.logXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

}}//end namespace
