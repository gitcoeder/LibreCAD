#include "lc_stepviewlayout.h"

#ifdef LC_HAVE_OCCT

#include <algorithm>
#include <limits>

#include "rs_color.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_pen.h"

namespace {

QString baseNameFor(LC_StepViewKind kind) {
    switch (kind) {
    case LC_StepViewKind::Front:
        return QStringLiteral("STEP-FRONT");
    case LC_StepViewKind::Top:
        return QStringLiteral("STEP-TOP");
    case LC_StepViewKind::Right:
        return QStringLiteral("STEP-RIGHT");
    case LC_StepViewKind::Iso:
    default:
        return QStringLiteral("STEP-ISO");
    }
}

struct Bounds {
    RS_Vector min{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};
    RS_Vector max{-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()};
    bool valid = false;

    void include(const RS_Entity* e) {
        RS_Vector eMin = e->getMin();
        RS_Vector eMax = e->getMax();
        min.x = std::min(min.x, eMin.x);
        min.y = std::min(min.y, eMin.y);
        max.x = std::max(max.x, eMax.x);
        max.y = std::max(max.y, eMax.y);
        valid = true;
    }

    RS_Vector size() const {
        return valid ? (max - min) : RS_Vector{0.0, 0.0};
    }
};

Bounds computeBounds(const LC_StepViewEntities& view) {
    Bounds b;
    for (RS_Entity* e : view.visible) {
        e->calculateBorders();
        b.include(e);
    }
    for (RS_Entity* e : view.hidden) {
        e->calculateBorders();
        b.include(e);
    }
    return b;
}

void translateView(LC_StepViewEntities& view, const RS_Vector& offset) {
    for (RS_Entity* e : view.visible) {
        e->move(offset);
        e->calculateBorders();
    }
    for (RS_Entity* e : view.hidden) {
        e->move(offset);
        e->calculateBorders();
    }
}

} // namespace

QString LC_StepViewLayout::layerNameFor(LC_StepViewKind kind, bool hidden) {
    QString name = baseNameFor(kind);
    if (hidden) {
        name += QStringLiteral("-HIDDEN");
    }
    return name;
}

RS_Layer* LC_StepViewLayout::ensureLayer(RS_Graphic& graphic, const QString& name, bool hidden) {
    if (RS_Layer* existing = graphic.findLayer(name)) {
        return existing;
    }
    auto* layer = new RS_Layer(name);
    if (hidden) {
        layer->setPen(RS_Pen(RS_Color(140, 140, 140), RS2::Width00, RS2::DashLine));
    } else {
        layer->setPen(RS_Pen(RS_Color(255, 255, 255), RS2::Width00, RS2::SolidLine));
    }
    graphic.addLayer(layer);
    return layer;
}

int LC_StepViewLayout::layoutAndInsert(RS_Graphic& graphic,
                                        const std::vector<LC_StepViewEntities>& viewsIn,
                                        LC_ProjectionAngle convention) {
    std::vector<LC_StepViewEntities> views = viewsIn;

    // Per-view raw bounds (before any placement offset).
    std::vector<Bounds> bounds;
    bounds.reserve(views.size());
    double maxDimension = 0.0;
    for (auto& view : views) {
        Bounds b = computeBounds(view);
        maxDimension = std::max({maxDimension, b.size().x, b.size().y});
        bounds.push_back(b);
    }
    // Fixed gap between views, scaled to the largest single-view dimension
    // so the layout stays legible regardless of overall part size.
    const double gap = std::max(20.0, maxDimension * 0.2);

    auto findView = [&views](LC_StepViewKind kind) -> LC_StepViewEntities* {
        for (auto& v : views) {
            if (v.kind == kind) {
                return &v;
            }
        }
        return nullptr;
    };
    auto findBounds = [&](LC_StepViewKind kind) -> Bounds {
        for (size_t i = 0; i < views.size(); ++i) {
            if (views[i].kind == kind) {
                return bounds[i];
            }
        }
        return {};
    };

    LC_StepViewEntities* front = findView(LC_StepViewKind::Front);
    LC_StepViewEntities* top = findView(LC_StepViewKind::Top);
    LC_StepViewEntities* right = findView(LC_StepViewKind::Right);
    LC_StepViewEntities* iso = findView(LC_StepViewKind::Iso);

    const Bounds frontB = findBounds(LC_StepViewKind::Front);
    const Bounds topB = findBounds(LC_StepViewKind::Top);
    const Bounds rightB = findBounds(LC_StepViewKind::Right);
    const Bounds isoB = findBounds(LC_StepViewKind::Iso);

    // Front view anchors the layout at the origin.
    if (front && frontB.valid) {
        translateView(*front, -frontB.min);
    }

    // Third-angle: top view sits above the front view; first-angle: below.
    if (top && topB.valid && frontB.valid) {
        const double verticalSign = (convention == LC_ProjectionAngle::Third) ? 1.0 : -1.0;
        const double targetY = (verticalSign > 0.0)
            ? (frontB.max.y - frontB.min.y + gap)
            : (-(topB.max.y - topB.min.y) - gap);
        RS_Vector offset{frontB.min.x - topB.min.x, targetY - topB.min.y};
        translateView(*top, offset);
    }

    // Right-side view sits to the right of the front view (both angle
    // conventions place it there; only top/bottom placement differs).
    if (right && rightB.valid && frontB.valid) {
        RS_Vector offset{
            frontB.max.x - frontB.min.x + gap - rightB.min.x,
            frontB.min.y - rightB.min.y
        };
        translateView(*right, offset);
    }

    // Isometric view placed above-right of the whole orthographic block.
    if (iso && isoB.valid && frontB.valid) {
        const double blockRight = (right && rightB.valid)
            ? (frontB.max.x - frontB.min.x + gap + (rightB.max.x - rightB.min.x))
            : (frontB.max.x - frontB.min.x);
        const double blockTop = (top && topB.valid)
            ? (frontB.max.y - frontB.min.y + gap + (topB.max.y - topB.min.y))
            : (frontB.max.y - frontB.min.y);
        RS_Vector offset{
            blockRight + gap - isoB.min.x,
            blockTop + gap - isoB.min.y
        };
        translateView(*iso, offset);
    }

    int inserted = 0;
    for (auto& view : views) {
        RS_Layer* visLayer = ensureLayer(graphic, layerNameFor(view.kind, false), false);
        RS_Layer* hidLayer = ensureLayer(graphic, layerNameFor(view.kind, true), true);
        for (RS_Entity* e : view.visible) {
            e->setLayer(visLayer);
            graphic.addEntity(e);
            ++inserted;
        }
        for (RS_Entity* e : view.hidden) {
            e->setLayer(hidLayer);
            graphic.addEntity(e);
            ++inserted;
        }
    }
    return inserted;
}

#endif // LC_HAVE_OCCT
