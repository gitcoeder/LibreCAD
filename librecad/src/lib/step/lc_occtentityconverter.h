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

#ifndef LC_OCCTENTITYCONVERTER_H
#define LC_OCCTENTITYCONVERTER_H

#ifdef LC_HAVE_OCCT

#include <vector>

#include <TopoDS_Shape.hxx>

class RS_Entity;
class RS_EntityContainer;

/**
 * Converts a compound of OCCT projected edges (the 2D-in-3D output of
 * LC_HlrProjector, one view's visible or hidden edge set) into unparented
 * RS_Entity objects: RS_Line/RS_Circle/RS_Arc/RS_Ellipse for edges whose
 * curve type has a direct LibreCAD equivalent, and a sampled RS_Line chain
 * (via adaptive deflection sampling) for anything else (B-splines, Beziers
 * -- fillets, blends, freeform surfaces). Mirrors the "return unparented
 * entities, caller attributes and inserts them" convention already used by
 * RS_FilterDXFRW::acisWireframeToEntities for the same kind of job.
 */
class LC_OcctEntityConverter {
public:
    struct Result {
        std::vector<RS_Entity*> entities;
        int approximatedEdgeCount = 0; //< edges that had no exact RS entity and were sampled
    };

    static Result convert(const TopoDS_Shape& edges, RS_EntityContainer* container);
};

#endif // LC_HAVE_OCCT
#endif // LC_OCCTENTITYCONVERTER_H
