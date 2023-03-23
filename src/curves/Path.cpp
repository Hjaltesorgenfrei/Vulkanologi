#include "Path.hpp"

Path::Path(bool loop) : loop(loop)
{
    points = std::vector<Point>();
    indices = std::vector<uint32_t>();
}

std::vector<uint32_t> const & Path::getIndices() const
{
    return indices;
}

std::vector<Point> const & Path::getPoints() const
{
    return points;
}

std::vector<FrenetFrame> const &Path::getFrenetFrames() const
{
    return frenetFrames;
}

void Path::recompute()
{
}

glm::mat4 frenetFrameMatrix(FrenetFrame frame)
{
    auto direction = frame.t;
    auto up = frame.n;
    auto pos = frame.o;
    auto z = glm::normalize(direction);
    auto x = glm::normalize(glm::cross(up, z));
    auto y = glm::normalize(glm::cross(z, x));
    return glm::mat4(
        x.x,   x.y,   x.z,   0.f,
        y.x,   y.y,   y.z,   0.f,
        z.x,   z.y,   z.z,   0.f,
        pos.x, pos.y, pos.z, 1.f
    );
}