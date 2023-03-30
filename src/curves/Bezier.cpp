#include "Bezier.hpp"
#include "Interpolation.hpp"
#include <algorithm>

Bezier::Bezier(glm::vec3 color) : color(color)
{
}

Bezier::Bezier(std::vector<std::shared_ptr<ControlPoint>> controlPoints, glm::vec3 color) : controlPoints(controlPoints), color(color)
{
    recompute();
}

std::vector<FrenetFrame> Bezier::getRMFrames(Point a, glm::vec3 b, glm::vec3 c, glm::vec3 d, std::vector<float>& evenTs)
{
    std::vector<FrenetFrame> frames;

    auto x0 = FrenetFrame{
        .o = a.position,
        .t = tangentCubicCurve(a.position, b, c, d, evenTs[0]),
        .n = a.normal};

    // Get right from normal and tangent
    x0.r = glm::normalize(glm::cross(x0.n, x0.t));

    frames.push_back(x0);

    for (int i = 1; i < evenTs.size(); i++)
    {
        x0 = frames.back();

        auto t1 = evenTs[i];
        auto x1 = FrenetFrame{.o = cubicCurve(a.position, b, c, d, t1), .t = tangentCubicCurve(a.position, b, c, d, t1)};

        auto v1 = x1.o - x0.o;
        auto c1 = glm::dot(v1, v1);
        auto niL = x0.n - v1 * 2.f / c1 * glm::dot(v1, x0.n);
        auto tiL = x0.t - v1 * 2.f / c1 * glm::dot(v1, x0.t);

        auto v2 = x1.t - tiL;
        auto c2 = glm::dot(v2, v2);

        x1.n = niL - v2 * 2.f / c2 * glm::dot(v2, niL);
        x1.r = glm::cross(x1.n, x1.t);
        frames.push_back(x1);
    }

    return frames;
}

void Bezier::recompute()
{
    if (controlPoints.size() < 2)
    {
        return;
    }

    points.clear();
    indices.clear();
    frenetFrames.clear();
    sortedFrames.clear();

    for (int i = 0; i < controlPoints.size() - 1; i++)
    {
        controlPoints[i]->dirty = false;
        controlPoints[i + 1]->dirty = false;
        auto a = controlPoints[i]->point();
        auto b = controlPoints[i]->forwardWorld();
        auto c = controlPoints[i + 1]->backwardWorld();
        auto d = controlPoints[i + 1]->point().position;

        auto evenTs = evenTsAlongCubic(a.position, b, c, d);
        auto evenPoints = evenPointsAlongCubicCurve(a.position, b, c, d, evenTs);
        auto frames = getRMFrames(a, b, c, d, evenTs);

        for (int j = 0; j < evenPoints.size(); j++)
        {
            auto point = evenPoints[j];
            auto frame = frames[j];
            auto normal = glm::normalize(frame.n);
            Point p {
                point, 
                normal, 
                color
            };
            points.push_back(p);

            if (j > 0)
            {
                indices.push_back(static_cast<uint32_t>(points.size() - 2));
                indices.push_back(static_cast<uint32_t>(points.size() - 1));
            }
        }

        frenetFrames.insert(frenetFrames.end(), frames.begin(), frames.end());

        assert(frames.size() == evenPoints.size());
        for (int j = 0; j < frames.size(); j++)
        {
            sortedFrames.emplace_back(evenTs[j], frames[j]);
        }
    }
}

void Bezier::addPoint(std::shared_ptr<ControlPoint> controlPoint)
{
    controlPoint->dirty = true;
    controlPoints.push_back(controlPoint);
    recompute();
}

std::shared_ptr<ControlPoint> Bezier::getPoint(int index) const
{
    return controlPoints.at(index);
}

size_t Bezier::getNumPoints()
{
    return controlPoints.size();
}

void Bezier::setPoint(int index, std::shared_ptr<ControlPoint> controlPoint)
{
    controlPoints.at(index) = controlPoint;
}

void Bezier::removePoint(int index)
{
    if (index < 0 || index >= controlPoints.size())
    {
        return;
    }

    auto pointToRemove = controlPoints.begin() + index;
    (*pointToRemove)->dirty = true;
    controlPoints.erase(controlPoints.begin() + index);
    recompute();
}

std::vector<std::shared_ptr<ControlPoint>> const &Bezier::getControlPoints() const
{
    return controlPoints;
}

FrenetFrame Bezier::frameAt(float t) const
{
    // find the first frame with a t greater than the given t
    auto it = std::upper_bound(sortedFrames.begin(), sortedFrames.end(), t, [](float t, const std::pair<float, FrenetFrame>& frame) {
        return t < frame.first;
    });
    // if the iterator is at the end, return the last frame
    if (it == sortedFrames.end())
    {
        return sortedFrames.back().second;
    }
    // if the iterator is at the beginning, return the first frame
    if (it == sortedFrames.begin())
    {
        return sortedFrames.front().second;
    }
    // otherwise, interpolate between the two frames
    
    auto t0 = (it - 1)->first;
    auto f0 = (it - 1)->second;
    auto t1 = it->first;
    auto f1 = it->second;
    auto tInterp = (t - t0) / (t1 - t0);
    return FrenetFrame {
        .o = glm::mix(f0.o, f1.o, tInterp),
        .t = glm::mix(f0.t, f1.t, tInterp),
        .r = glm::mix(f0.r, f1.r, tInterp),
        .n = glm::mix(f0.n, f1.n, tInterp)
    };
}

std::vector<glm::vec3> Bezier::evenPointsAlongCubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, std::vector<float>& evenTs)
{
    std::vector<glm::vec3> points;

    for (auto t : evenTs)
    {
        points.push_back(cubicCurve(a, b, c, d, t));
    }

    return points;
}

std::vector<float> Bezier::evenTsAlongCubic(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d)
{
    std::vector<float> ts;

    auto lengths = arcLength(a, b, c, d);
    auto length = lengths.back().first;

    float currentLength = 0.f;
    int currentSegment = 0;
    for (int i = 0; i < lengths.size(); i++)
    {
        if (lengths[i].first > currentLength)
        {
            ts.push_back(lengths[i].second);
            currentSegment++;
            currentLength = resolution * (float)currentSegment;
        }
    }

    return ts;
}

std::vector<std::pair<float, float>> Bezier::arcLength(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d)
{
    std::vector<std::pair<float, float>> lengths;
    int steps = static_cast<int>(estimateArcLength(a, b, c, d) / resolution) * 5;
    float length = 0.f;
    glm::vec3 lastPoint = a;
    for (int i = 1; i <= steps; i++)
    {
        float t = (float)i / (float)steps;
        auto point = cubicCurve(a, b, c, d, t);
        length += glm::length(point - lastPoint);
        lastPoint = point;
        lengths.push_back(std::make_pair(length, t));
    }
    return lengths;
}

float Bezier::estimateArcLength(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d)
{
    // Length between a and d plus half of the control net
    return glm::length(a - d) + 0.5f * (glm::length(a - b) + glm::length(b - c) + glm::length(c - d));
}

bool Bezier::recomputeIfDirty()
{
    if (std::any_of(controlPoints.begin(), controlPoints.end(), [](std::shared_ptr<ControlPoint> const &p) { return p->dirty; }))
    {
        recompute();
        return true;
    }
    return false;
}
