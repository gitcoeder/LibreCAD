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

#ifndef LC_STEPVIEWLAYOUT_H
#define LC_STEPVIEWLAYOUT_H

#ifdef LC_HAVE_OCCT

#include <vector>

#include <QString>

#include "lc_hlrprojector.h"
#include "rs_vector.h"

class RS_Entity;
class RS_Graphic;
class RS_Layer;

/**
 * Projection convention used to arrange the generated views. Third-angle
 * (ANSI/US) is the default; kept as an enum rather than hardcoded so a
 * first-angle (ISO/European) mode is a cheap follow-up.
 */
enum class LC_ProjectionAngle {
    First,
    Third
};

/**
 * One generated view's entities, split into visible and hidden groups, still
 * at their raw (un-offset) projection-plane coordinates.
 */
struct LC_StepViewEntities {
    LC_StepViewKind kind;
    std::vector<RS_Entity*> visible;
    std::vector<RS_Entity*> hidden;
};

/**
 * Computes a non-overlapping grid layout for the 4 generated views (third-
 * angle "glass box" unfold by default), creates the 8 per-view layers
 * (visible + hidden per view) on the target graphic, translates each view's
 * entities into place, and inserts everything into the graphic.
 */
class LC_StepViewLayout {
public:
    static QString layerNameFor(LC_StepViewKind kind, bool hidden);

    // Lays out and inserts all views' entities into `graphic`, creating
    // layers as needed. Returns the total number of entities inserted.
    static int layoutAndInsert(RS_Graphic& graphic,
                                const std::vector<LC_StepViewEntities>& views,
                                LC_ProjectionAngle convention = LC_ProjectionAngle::Third);

private:
    static RS_Layer* ensureLayer(RS_Graphic& graphic, const QString& name, bool hidden);
};

#endif // LC_HAVE_OCCT
#endif // LC_STEPVIEWLAYOUT_H
