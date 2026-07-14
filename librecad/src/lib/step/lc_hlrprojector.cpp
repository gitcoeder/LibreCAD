#include "lc_hlrprojector.h"

#ifdef LC_HAVE_OCCT

#include <algorithm>
#include <cmath>

#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Bnd_Box.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <HLRBRep_PolyAlgo.hxx>
#include <HLRBRep_PolyHLRToShape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace {

// View direction (N) + horizontal axis (Vx) for each standard view, in the
// STEP model's own coordinate system (Z-up assumed, the common CAD
// convention). Third-angle projection: top view unfolds above the front
// view, right-side view unfolds to the right of the front view.
gp_Ax2 axisForView(LC_StepViewKind view) {
    switch (view) {
    case LC_StepViewKind::Front:
        // Looking in -Y, horizontal = +X, vertical = +Z.
        return gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(0, -1, 0), gp_Dir(1, 0, 0));
    case LC_StepViewKind::Top:
        // Looking in -Z (down), horizontal = +X, vertical = +Y.
        return gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(0, 0, -1), gp_Dir(1, 0, 0));
    case LC_StepViewKind::Right:
        // Looking in -X (from the right side), horizontal = +Y, vertical = +Z.
        return gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(-1, 0, 0), gp_Dir(0, 1, 0));
    case LC_StepViewKind::Iso:
    default:
        // Standard isometric direction (1,1,1), looking toward the origin.
        return gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(-1, -1, -1), gp_Dir(1, -1, 0));
    }
}

} // namespace

LC_HlrProjector::LC_HlrProjector(const TopoDS_Shape& shape)
    : m_shape(shape) {
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        ++m_faceCount;
    }
    m_fastMode = m_faceCount >= kFastModeFaceThreshold;

    if (m_fastMode) {
        // Chord-height meshing tolerance scaled to model size: fine enough
        // that the polygonal projection reads cleanly at drawing scale,
        // coarse enough that a large assembly meshes in about a second.
        Bnd_Box box;
        BRepBndLib::Add(shape, box);
        Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
        box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        const double dx = xmax - xmin, dy = ymax - ymin, dz = zmax - zmin;
        const double diag = std::sqrt(dx * dx + dy * dy + dz * dz);
        const double deflection = std::max(0.01, diag * 0.001);

        // Meshing attaches triangulation to the shape itself, so this
        // one-time cost is shared by all four per-view projections.
        BRepMesh_IncrementalMesh mesher(m_shape, deflection);
    }
}

LC_HlrProjector::~LC_HlrProjector() = default;

// A fresh algorithm instance per view: reusing one across projector changes
// leaks edge-splitting/visibility state between views and subtly corrupts
// the classification (observed as hidden edges reported visible, and full
// circles split into arcs). The per-view algorithm run is the cheap part;
// the expensive shared setup (meshing) already happened in the constructor.
LC_HlrViewResult LC_HlrProjector::project(LC_StepViewKind view) {
    LC_HlrViewResult result;
    const HLRAlgo_Projector projector(axisForView(view));

    if (m_fastMode) {
        Handle(HLRBRep_PolyAlgo) algo = new HLRBRep_PolyAlgo();
        algo->Load(m_shape);
        algo->Projector(projector);
        algo->Update();

        HLRBRep_PolyHLRToShape extractor;
        extractor.Update(algo);
        result.visibleSharp = extractor.VCompound();
        result.visibleOutline = extractor.OutLineVCompound();
        result.hiddenSharp = extractor.HCompound();
        result.hiddenOutline = extractor.OutLineHCompound();
    } else {
        Handle(HLRBRep_Algo) algo = new HLRBRep_Algo();
        algo->Add(m_shape);
        algo->Projector(projector);
        algo->Update();
        algo->Hide();

        HLRBRep_HLRToShape extractor(algo);
        result.visibleSharp = extractor.VCompound();
        result.visibleOutline = extractor.OutLineVCompound();
        result.hiddenSharp = extractor.HCompound();
        result.hiddenOutline = extractor.OutLineHCompound();
    }

    return result;
}

#endif // LC_HAVE_OCCT
