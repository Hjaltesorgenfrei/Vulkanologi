#include "Path.h"

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

std::vector<ControlPoint> const &Path::getControlPoints() const
{
    return controlPoints;
}

void Path::generateFrenetFrames()
{
}

void Path::recompute()
{
}
