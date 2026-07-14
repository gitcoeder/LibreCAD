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

#ifndef LC_HLRPROJECTOR_H
#define LC_HLRPROJECTOR_H

#ifdef LC_HAVE_OCCT

#include <TopoDS_Shape.hxx>

/**
 * The four standard views generated for every STEP import.
 */
enum class LC_StepViewKind {
    Front,
    Top,
    Right,
    Iso
};

/**
 * Result of a single hidden-line-removal projection: visible and hidden
 * edge geometry, kept separate (sharp edges vs. smooth-surface silhouette
 * "outline" edges -- both are needed, since silhouette edges are what make
 * cylinders/fillets show up as curves at all rather than edgeless blobs).
 * All edges are 2D-in-3D (Z ~ 0) in the projection plane's own coordinate
 * system, i.e. ready for LC_OcctEntityConverter to read as (u, v) pairs.
 */
struct LC_HlrViewResult {
    TopoDS_Shape visibleSharp;
    TopoDS_Shape visibleOutline;
    TopoDS_Shape hiddenSharp;
    TopoDS_Shape hiddenOutline;
};

/**
 * Computes 2D orthographic/isometric projections of a 3D shape with hidden
 * line removal. Two modes, selected automatically by model complexity:
 *
 * - Exact (HLRBRep_Algo): projected edges stay typed as analytic curves
 *   (lines/circles/ellipses), so the entity converter can emit native
 *   RS_Circle/RS_Arc entities. Accurate but slow -- runtime grows badly
 *   with face count and made complex assemblies hang for minutes
 *   (gitcoeder/LibreCAD#1).
 *
 * - Fast polygonal (HLRBRep_PolyAlgo over a triangulated mesh): all output
 *   is straight-segment chains, so curves become polylines, but a
 *   2000+-face assembly projects in well under a second per view.
 *
 * Construct once per import; the expensive setup (shape load / meshing)
 * happens in the constructor and is shared by all four project() calls.
 */
class LC_HlrProjector {
public:
    explicit LC_HlrProjector(const TopoDS_Shape& shape);
    ~LC_HlrProjector();

    LC_HlrViewResult project(LC_StepViewKind view);

    //! True when model complexity forced the fast polygonal mode, meaning
    //! all curved edges in the result are straight-segment approximations.
    bool usesFastApproximation() const { return m_fastMode; }

    int faceCount() const { return m_faceCount; }

    //! Face count at or above which the fast polygonal algorithm is used.
    static constexpr int kFastModeFaceThreshold = 400;

private:
    TopoDS_Shape m_shape;
    int m_faceCount = 0;
    bool m_fastMode = false;
};

#endif // LC_HAVE_OCCT
#endif // LC_HLRPROJECTOR_H
