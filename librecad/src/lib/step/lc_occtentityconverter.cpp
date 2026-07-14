#include "lc_occtentityconverter.h"

#ifdef LC_HAVE_OCCT

#include <cmath>

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_vector.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <GeomAbs_CurveType.hxx>
#include <Geom_Curve.hxx>
#include <Poly_Polygon3D.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <Precision.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

namespace {

constexpr double TWO_PI = 2.0 * M_PI;
constexpr double FULL_PARAM_TOL = 1.0e-6;
// Chord-height tolerance (drawing units, i.e. mm after LC_StepReader's unit
// normalization) used when sampling curve types with no exact RS entity.
constexpr double SAMPLING_DEFLECTION = 0.1;

RS_Vector toVector2d(const gp_Pnt& p) {
    return {p.X(), p.Y()};
}

// True if travelling counter-clockwise from `from` to `to` passes through `mid`.
bool isCcwSweepThrough(double from, double mid, double to) {
    auto normSweep = [](double a, double b) {
        double d = std::fmod(b - a, TWO_PI);
        if (d < 0.0) {
            d += TWO_PI;
        }
        return d;
    };
    return normSweep(from, mid) <= normSweep(from, to);
}

void appendSampledPolyline(const BRepAdaptor_Curve& curve, RS_EntityContainer* container,
                            std::vector<RS_Entity*>& out) {
    GCPnts_UniformDeflection sampler(curve, SAMPLING_DEFLECTION);
    if (!sampler.IsDone() || sampler.NbPoints() < 2) {
        // Degenerate/very short edge: fall back to a single straight segment.
        RS_Vector p1 = toVector2d(curve.Value(curve.FirstParameter()));
        RS_Vector p2 = toVector2d(curve.Value(curve.LastParameter()));
        out.push_back(new RS_Line(container, RS_LineData{p1, p2}));
        return;
    }
    for (int i = 1; i <= sampler.NbPoints() - 1; ++i) {
        RS_Vector p1 = toVector2d(sampler.Value(i));
        RS_Vector p2 = toVector2d(sampler.Value(i + 1));
        out.push_back(new RS_Line(container, RS_LineData{p1, p2}));
    }
}

// Edges produced by the fast polygonal HLR path (HLRBRep_PolyAlgo) carry no
// geometric curve at all, only a Poly_Polygon3D of already-projected points.
// Returns true if the edge was consumed as such.
bool convertPolygonalEdge(const TopoDS_Edge& edge, RS_EntityContainer* container,
                           std::vector<RS_Entity*>& out) {
    Standard_Real first, last;
    if (!BRep_Tool::Curve(edge, first, last).IsNull()) {
        return false; // has a real 3D curve; use the analytic path
    }
    TopLoc_Location location;
    Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(edge, location);
    if (polygon.IsNull() || polygon->NbNodes() < 2) {
        // No 3D curve but also no polygon: exact-HLR output edges store
        // their geometry as a 2D curve-on-surface, which BRepAdaptor_Curve
        // resolves fine -- fall through to the analytic path.
        return false;
    }
    const TColgp_Array1OfPnt& nodes = polygon->Nodes();
    const bool transform = !location.IsIdentity();
    const gp_Trsf trsf = location.Transformation();
    for (int i = nodes.Lower(); i < nodes.Upper(); ++i) {
        gp_Pnt a = nodes(i);
        gp_Pnt b = nodes(i + 1);
        if (transform) {
            a.Transform(trsf);
            b.Transform(trsf);
        }
        out.push_back(new RS_Line(container, RS_LineData{toVector2d(a), toVector2d(b)}));
    }
    return true;
}

void convertEdge(const TopoDS_Edge& edge, RS_EntityContainer* container,
                  std::vector<RS_Entity*>& out, int& approximatedCount) {
    if (convertPolygonalEdge(edge, container, out)) {
        return;
    }

    BRepAdaptor_Curve curve(edge);
    const double first = curve.FirstParameter();
    const double last = curve.LastParameter();
    if (last - first < Precision::Confusion()) {
        return; // degenerate edge
    }

    switch (curve.GetType()) {
    case GeomAbs_Line: {
        RS_Vector p1 = toVector2d(curve.Value(first));
        RS_Vector p2 = toVector2d(curve.Value(last));
        out.push_back(new RS_Line(container, RS_LineData{p1, p2}));
        break;
    }
    case GeomAbs_Circle: {
        gp_Circ circ = curve.Circle();
        RS_Vector center = toVector2d(circ.Location());
        const double radius = circ.Radius();

        if (std::fabs((last - first) - TWO_PI) < FULL_PARAM_TOL) {
            out.push_back(new RS_Circle(container, RS_CircleData(center, radius)));
            break;
        }

        RS_Vector p1 = toVector2d(curve.Value(first));
        RS_Vector p2 = toVector2d(curve.Value(last));
        RS_Vector pMid = toVector2d(curve.Value((first + last) / 2.0));
        const double angle1 = (p1 - center).angle();
        const double angle2 = (p2 - center).angle();
        const double angleMid = (pMid - center).angle();
        const bool reversed = !isCcwSweepThrough(angle1, angleMid, angle2);
        out.push_back(new RS_Arc(container, RS_ArcData(center, radius, angle1, angle2, reversed)));
        break;
    }
    case GeomAbs_Ellipse: {
        gp_Elips elips = curve.Ellipse();
        RS_Vector center = toVector2d(elips.Location());
        const double majorRadius = elips.MajorRadius();
        const double minorRadius = elips.MinorRadius();
        gp_Pnt majorAxisPoint = elips.Location().Translated(
            gp_Vec(elips.XAxis().Direction()) * majorRadius);
        RS_Vector majorP = toVector2d(majorAxisPoint) - center;
        const double ratio = (majorRadius > Precision::Confusion()) ? (minorRadius / majorRadius) : 0.0;

        RS_EllipseData data;
        data.center = center;
        data.majorP = majorP;
        data.ratio = ratio;

        if (std::fabs((last - first) - TWO_PI) < FULL_PARAM_TOL) {
            data.angle1 = 0.0;
            data.angle2 = 0.0;
            data.reversed = false;
        } else {
            RS_Vector pMid3d = toVector2d(curve.Value((first + last) / 2.0));
            // Parametric angle for full-ellipse test is in the curve's own
            // local frame, same convention DXF ellipse import already uses.
            data.angle1 = first;
            data.angle2 = last;
            data.reversed = !isCcwSweepThrough(first, (first + last) / 2.0, last);
            (void)pMid3d;
        }
        out.push_back(new RS_Ellipse(container, data));
        break;
    }
    default:
        // B-spline/Bezier/hyperbola/parabola/other -- no exact RS entity;
        // approximate with a sampled polyline and count it for the
        // user-facing import summary.
        appendSampledPolyline(curve, container, out);
        ++approximatedCount;
        break;
    }
}

} // namespace

LC_OcctEntityConverter::Result LC_OcctEntityConverter::convert(
    const TopoDS_Shape& edges, RS_EntityContainer* container) {
    Result result;
    if (edges.IsNull()) {
        return result;
    }

    for (TopExp_Explorer exp(edges, TopAbs_EDGE); exp.More(); exp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(exp.Current());
        convertEdge(edge, container, result.entities, result.approximatedEdgeCount);
    }

    return result;
}

#endif // LC_HAVE_OCCT
