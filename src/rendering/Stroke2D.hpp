#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "ofColor.h"
#include "ofMesh.h"
#include "ofPolyline.h"

namespace ofxMarkSynth {

/// Builds a 2D stroke mesh from an ofPolyline.
///
/// - 2D only (XY; Z ignored)
/// - Constant width + constant color
/// - Closed polylines support inside/center/outside alignment
/// - Miter joins with a clamp (miterLimit)
class Stroke2D {
public:
    enum class Alignment {
        Center,
        Outside,
        Inside,
    };

    struct Params {
        float strokeWidth { 1.0f };
        float feather { 0.0f };
        float miterLimit { 4.0f };
        Alignment alignment { Alignment::Center };

        /// Feather on the positive offset side.
        /// For closed polylines, "positive" means "outside".
        bool featherPositive { true };

        /// Feather on the negative offset side.
        /// For closed polylines, "negative" means "inside".
        bool featherNegative { true };

        float epsilon { 1e-6f };
    };

    Stroke2D() = default;

    void setParams(const Params& p) { params = p; }
    const Params& getParams() const { return params; }

    void setColor(const ofFloatColor& c) { color = c; }
    const ofFloatColor& getColor() const { return color; }

    /// Build mesh from polyline. Returns false if polyline is degenerate.
    bool build(const ofPolyline& poly);

    const ofMesh& getMesh() const { return mesh; }
    void draw() const;

private:
    static glm::vec2 perpendicular(const glm::vec2& v);
    static float signedArea(const std::vector<glm::vec2>& pts);
    static float lengthSquared(const glm::vec2& v);
    static glm::vec2 normalizeOrZero(const glm::vec2& v, float eps);

    bool buildFromPoints(const std::vector<glm::vec2>& pts, bool closed);

    Params params;
    ofFloatColor color { 1.0f, 1.0f, 1.0f, 1.0f };

    ofMesh mesh;
    std::vector<glm::vec2> points;
};

} // namespace ofxMarkSynth
