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

#ifndef RS_FILTERSTEP_H
#define RS_FILTERSTEP_H

#include "rs_filterinterface.h"

/**
 * Format filter that imports STEP (ISO-10303) 3D files by computing
 * standard 2D orthographic + isometric views (with hidden-line removal)
 * from the 3D solid and inserting them as native, editable LibreCAD
 * entities. Requires an OpenCASCADE-enabled build (LC_HAVE_OCCT); when
 * built without OpenCASCADE, fileImport() fails cleanly with an
 * explanatory lastError().
 *
 * Export is not supported.
 */
class RS_FilterSTEP : public RS_FilterInterface {
public:
    RS_FilterSTEP() = default;

    RS2::FormatType rtti() const {
        return RS2::FormatSTEP;
    }

    bool canImport(const QString& /*fileName*/, RS2::FormatType t) const override {
        return t == RS2::FormatSTEP;
    }

    bool canExport(const QString& /*fileName*/, RS2::FormatType /*t*/) const override {
        return false;
    }

    bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType type) override;

    bool fileExport(RS_Graphic& /*g*/, const QString& /*file*/, RS2::FormatType /*type*/) override {
        return false;
    }

    QString lastError() const override {
        return m_lastError;
    }

    static RS_FilterInterface* createFilter() {
        return new RS_FilterSTEP();
    }

private:
    QString m_lastError;
};

#endif
