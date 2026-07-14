#include "lc_hlrprojector.h"

#ifdef LC_HAVE_OCCT

#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRBRep_HLRToShape.hxx>
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

LC_HlrViewResult LC_HlrProjector::project(const TopoDS_Shape& shape, LC_StepViewKind view) {
    LC_HlrViewResult result;

    Handle(HLRBRep_Algo) algo = new HLRBRep_Algo();
    algo->Add(shape);

    HLRAlgo_Projector projector(axisForView(view));
    algo->Projector(projector);
    algo->Update();
    algo->Hide();

    HLRBRep_HLRToShape extractor(algo);

    result.visibleSharp = extractor.VCompound();
    result.visibleOutline = extractor.OutLineVCompound();
    result.hiddenSharp = extractor.HCompound();
    result.hiddenOutline = extractor.OutLineHCompound();

    return result;
}

#endif // LC_HAVE_OCCT
