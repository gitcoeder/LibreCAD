/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_filterstep.h"

#include "rs_debug.h"
#include "rs_graphic.h"

#ifdef LC_HAVE_OCCT
#include "lc_hlrprojector.h"
#include "lc_occtentityconverter.h"
#include "lc_stepreader.h"
#include "lc_stepviewlayout.h"
#endif

bool RS_FilterSTEP::fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) {
    RS_DEBUG->print("RS_FilterSTEP::fileImport: %s", file.toLatin1().data());

#ifndef LC_HAVE_OCCT
    m_lastError = QObject::tr(
        "STEP import requires an OpenCASCADE-enabled build of LibreCAD.",
        "RS_FilterSTEP");
    return false;
#else
    LC_StepReader::Result read = LC_StepReader::read(file);
    if (!read.success) {
        m_lastError = read.errorMessage;
        return false;
    }

    g.setUnit(RS2::Millimeter);

    static constexpr LC_StepViewKind kAllViews[] = {
        LC_StepViewKind::Front, LC_StepViewKind::Top,
        LC_StepViewKind::Right, LC_StepViewKind::Iso
    };

    std::vector<LC_StepViewEntities> views;
    int approximatedEdgeCount = 0;
    for (LC_StepViewKind kind : kAllViews) {
        LC_HlrViewResult hlr = LC_HlrProjector::project(read.shape, kind);

        LC_StepViewEntities view;
        view.kind = kind;

        auto appendConverted = [&](const TopoDS_Shape& edges, std::vector<RS_Entity*>& dest) {
            LC_OcctEntityConverter::Result converted = LC_OcctEntityConverter::convert(edges, &g);
            dest.insert(dest.end(), converted.entities.begin(), converted.entities.end());
            approximatedEdgeCount += converted.approximatedEdgeCount;
        };

        appendConverted(hlr.visibleSharp, view.visible);
        appendConverted(hlr.visibleOutline, view.visible);
        appendConverted(hlr.hiddenSharp, view.hidden);
        appendConverted(hlr.hiddenOutline, view.hidden);

        views.push_back(std::move(view));
    }

    const int insertedCount = LC_StepViewLayout::layoutAndInsert(g, views);
    if (insertedCount == 0) {
        m_lastError = QObject::tr(
            "STEP file was read but produced no visible geometry in any view: %1",
            "RS_FilterSTEP").arg(file);
        return false;
    }

    if (approximatedEdgeCount > 0) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
            "RS_FilterSTEP::fileImport: %d edge(s) had no exact match and were "
            "approximated as polylines", approximatedEdgeCount);
    }

    return true;
#endif
}
