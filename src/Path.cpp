#include "Path.h"

Path::Path(bool loop) : loop(loop)
{
    points = std::vector<Point>();
    indices = std::vector<uint32_t>();
}

void Path::addPoint(Point point)
{
    points.push_back(point);

    if (points.size() == 1) {
        return;
    }

    uint32_t last = static_cast<uint32_t>(points.size() - 1);
    
    if (loop) {
        if (points.size() == 2) {
            indices.push_back(0);
            indices.push_back(1);
        }

        indices[indices.size() - 2] = last - 1;
        indices[indices.size() - 1] = last;

        indices.push_back(last);
        indices.push_back(0);
    }
    else {
        indices.push_back(last - 1);
        indices.push_back(last);
    }
}

Point Path::getPoint(int index)
{
    return points.at(index);
}

size_t Path::getNumPoints()
{
    return points.size();
}

void Path::setPoint(int index, Point point)
{
    points.at(index) = point;
}

void Path::removePoint(int index)
{
    if (index < 0 || index >= points.size()) {
        return;
    }
    
    points.erase(points.begin() + index);

    if (!indices.size()) { 
        return;
    }   
    indices.pop_back();
    indices.pop_back();
    if (loop) {
        uint32_t last = static_cast<uint32_t>(points.size() - 1);
        indices[indices.size() - 2] = last - 1;
        indices[indices.size() - 1] = last;
    }
}

void Path::clear()
{
    points.clear();
    indices.clear();
}

std::vector<uint32_t> const & Path::getIndices() const
{
    return indices;
}

std::vector<Point> const & Path::getPoints() const
{
    return points;
}

Path linePath(glm::vec3 start, glm::vec3 end, glm::vec3 color)
{
    Path path;

    glm::vec3 up = glm::vec3(0, 1, 0);
    path.addPoint({ start, color, up });
    path.addPoint({ end, color, up });

    return path;
}
