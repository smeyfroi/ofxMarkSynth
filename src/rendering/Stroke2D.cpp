#include "rendering/Stroke2D.hpp"

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace ofxMarkSynth {

namespace {

inline glm::vec2 toVec2(const glm::vec3& v) {
    return { v.x, v.y };
}

inline glm::vec3 toVec3(const glm::vec2& v) {
    return { v.x, v.y, 0.0f };
}

void addQuad(std::vector<ofIndexType>& indices, ofIndexType a, ofIndexType b, ofIndexType c, ofIndexType d) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);

    indices.push_back(c);
    indices.push_back(b);
    indices.push_back(d);
}

glm::vec2 clampMiterOffset(glm::vec2 offset, float limit) {
    if (limit <= 0.0f) return offset;

    float len2 = offset.x * offset.x + offset.y * offset.y;
    if (len2 <= limit * limit) return offset;

    float len = std::sqrt(len2);
    if (len <= 0.0f) return {};

    return offset * (limit / len);
}

} // namespace

glm::vec2 Stroke2D::perpendicular(const glm::vec2& v) {
    return { -v.y, v.x };
}

float Stroke2D::lengthSquared(const glm::vec2& v) {
    return v.x * v.x + v.y * v.y;
}

glm::vec2 Stroke2D::normalizeOrZero(const glm::vec2& v, float eps) {
    float len2 = lengthSquared(v);
    if (len2 <= eps * eps) return { 0.0f, 0.0f };
    return v * (1.0f / std::sqrt(len2));
}

float Stroke2D::signedArea(const std::vector<glm::vec2>& pts) {
    if (pts.size() < 3) return 0.0f;

    double a = 0.0;
    for (size_t i = 0; i < pts.size(); ++i) {
        const auto& p0 = pts[i];
        const auto& p1 = pts[(i + 1) % pts.size()];
        a += static_cast<double>(p0.x) * static_cast<double>(p1.y) - static_cast<double>(p1.x) * static_cast<double>(p0.y);
    }
    return static_cast<float>(0.5 * a);
}

bool Stroke2D::build(const ofPolyline& poly) {
    points.clear();

    const auto& verts = poly.getVertices();
    if (verts.empty()) return false;

    float eps2 = params.epsilon * params.epsilon;
    points.reserve(verts.size());

    for (const auto& v : verts) {
        glm::vec2 p = toVec2(v);
        if (points.empty() || lengthSquared(p - points.back()) > eps2) {
            points.push_back(p);
        }
    }

    if (points.size() < 2) return false;

    bool closed = poly.isClosed();

    // If the first point is duplicated at the end, treat as closed and remove the duplicate.
    if (lengthSquared(points.front() - points.back()) <= eps2) {
        closed = true;
        points.pop_back();
    }

    if (closed) {
        if (points.size() < 3) return false;
    } else {
        if (points.size() < 2) return false;
    }

    return buildFromPoints(points, closed);
}

bool Stroke2D::buildFromPoints(const std::vector<glm::vec2>& pts, bool closed) {
    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);

    if (params.strokeWidth <= 0.0f) return false;

    const float halfWidth = params.strokeWidth * 0.5f;

    Alignment alignment = params.alignment;
    if (!closed) {
        // Inside/outside are ambiguous for open paths.
        alignment = Alignment::Center;
    }

    float posDist = 0.0f;
    float negDist = 0.0f;
    switch (alignment) {
        case Alignment::Center:
            posDist = halfWidth;
            negDist = -halfWidth;
            break;
        case Alignment::Outside:
            posDist = params.strokeWidth;
            negDist = 0.0f;
            break;
        case Alignment::Inside:
            posDist = 0.0f;
            negDist = -params.strokeWidth;
            break;
    }

    float featherPos = params.featherPositive ? params.feather : 0.0f;
    float featherNeg = params.featherNegative ? params.feather : 0.0f;
    bool useFeather = (featherPos > 0.0f) || (featherNeg > 0.0f);

    float outwardSign = 1.0f;
    if (closed) {
        // For CCW polygons, the interior is on the left side of the tangent, so outward is -leftNormal.
        float area = signedArea(pts);
        if (std::abs(area) > params.epsilon) {
            outwardSign = (area < 0.0f) ? 1.0f : -1.0f;
        }
    }

    const size_t n = pts.size();

    std::vector<glm::vec3> vertices;
    std::vector<ofFloatColor> colors;
    std::vector<ofIndexType> indices;

    int vertsPerPoint = useFeather ? 4 : 2;
    vertices.reserve(n * vertsPerPoint);
    colors.reserve(n * vertsPerPoint);

    auto baseColor = color;
    auto featherColor = color;
    featherColor.a = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        size_t prev = (i == 0) ? (closed ? n - 1 : 0) : i - 1;
        size_t next = (i + 1 >= n) ? (closed ? 0 : n - 1) : i + 1;

        glm::vec2 p = pts[i];

        glm::vec2 d0 = pts[i] - pts[prev];
        glm::vec2 d1 = pts[next] - pts[i];

        d0 = normalizeOrZero(d0, params.epsilon);
        d1 = normalizeOrZero(d1, params.epsilon);

        if (lengthSquared(d0) <= params.epsilon * params.epsilon) d0 = d1;
        if (lengthSquared(d1) <= params.epsilon * params.epsilon) d1 = d0;

        glm::vec2 n0 = perpendicular(d0);
        glm::vec2 n1 = perpendicular(d1);

        if (closed) {
            n0 *= outwardSign;
            n1 *= outwardSign;
        }

        n0 = normalizeOrZero(n0, params.epsilon);
        n1 = normalizeOrZero(n1, params.epsilon);

        glm::vec2 basis = n1;

        bool isEndpoint = !closed && (i == 0 || i == n - 1);
        if (!isEndpoint) {
            glm::vec2 sum = n0 + n1;
            if (lengthSquared(sum) > params.epsilon * params.epsilon) {
                glm::vec2 m = normalizeOrZero(sum, params.epsilon);
                float denom = glm::dot(m, n1);
                if (std::abs(denom) > params.epsilon) {
                    basis = m * (1.0f / denom);
                }
            }
        }

        auto offsetFor = [&](float dist) {
            if (std::abs(dist) <= params.epsilon) return glm::vec2 { 0.0f, 0.0f };

            glm::vec2 offset = basis * dist;
            float limit = params.miterLimit * std::abs(dist);
            return clampMiterOffset(offset, limit);
        };

        glm::vec2 posEdge = p + offsetFor(posDist);
        glm::vec2 negEdge = p + offsetFor(negDist);

        if (useFeather) {
            glm::vec2 posOuter = p + offsetFor(posDist + featherPos);
            glm::vec2 negOuter = p + offsetFor(negDist - featherNeg);

            vertices.push_back(toVec3(posOuter));
            colors.push_back(featherColor);

            vertices.push_back(toVec3(posEdge));
            colors.push_back(baseColor);

            vertices.push_back(toVec3(negEdge));
            colors.push_back(baseColor);

            vertices.push_back(toVec3(negOuter));
            colors.push_back(featherColor);
        } else {
            vertices.push_back(toVec3(posEdge));
            colors.push_back(baseColor);

            vertices.push_back(toVec3(negEdge));
            colors.push_back(baseColor);
        }
    }

    size_t segCount = closed ? n : (n - 1);
    if (segCount == 0) return false;

    indices.reserve(segCount * (useFeather ? 18 : 6));

    for (size_t i = 0; i < segCount; ++i) {
        size_t j = (i + 1) % n;

        ofIndexType baseI = static_cast<ofIndexType>(i * vertsPerPoint);
        ofIndexType baseJ = static_cast<ofIndexType>(j * vertsPerPoint);

        if (useFeather) {
            if (featherPos > 0.0f) {
                addQuad(indices, baseI + 0, baseI + 1, baseJ + 0, baseJ + 1);
            }

            addQuad(indices, baseI + 1, baseI + 2, baseJ + 1, baseJ + 2);

            if (featherNeg > 0.0f) {
                addQuad(indices, baseI + 2, baseI + 3, baseJ + 2, baseJ + 3);
            }
        } else {
            addQuad(indices, baseI + 0, baseI + 1, baseJ + 0, baseJ + 1);
        }
    }

    mesh.addVertices(vertices);
    mesh.addColors(colors);
    mesh.addIndices(indices);

    return true;
}

void Stroke2D::draw() const {
    mesh.draw();
}

} // namespace ofxMarkSynth
