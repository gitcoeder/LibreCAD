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

#ifndef LC_STEPREADER_H
#define LC_STEPREADER_H

#ifdef LC_HAVE_OCCT

#include <QString>
#include <TopoDS_Shape.hxx>

/**
 * Thin wrapper around OCCT's STEPControl_Reader. Loads a STEP file into a
 * single combined TopoDS_Shape (OCCT unions multiple transferred roots into
 * a compound automatically). Always normalizes the working unit to
 * millimeters (via xstep.cascade.unit) so callers don't need to interpret
 * whatever unit the STEP file itself declares.
 */
class LC_StepReader {
public:
    struct Result {
        bool success = false;
        TopoDS_Shape shape;
        int rootCount = 0;
        QString errorMessage;
    };

    static Result read(const QString& fileName);
};

#endif // LC_HAVE_OCCT
#endif // LC_STEPREADER_H
