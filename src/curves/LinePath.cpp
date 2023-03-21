#include "LinePath.h"

LinePath::LinePath(glm::vec3 start, glm::vec3 end, glm::vec3 color)
{
    // Normal based on tangent between the two points
    addPoint(start, color);
    addPoint(end, color);
}

void LinePath::generateFrenetFrames()
{
    // TODO: Implement by linear interpolation between points, based on total distance.
}

void LinePath::addPoint(glm::vec3 position, glm::vec3 color)
{
    if (points.size() == 0) {
        addPoint({ position, glm::vec3(0, 1, 0), color });
        return;
    }

    Point& previous = points.back();

    if (points.size() == 1) {
        glm::vec3 binormal = glm::normalize(glm::cross(glm::vec3(0, 1, 0), position - previous.position));
        glm::vec3 normal = glm::normalize(glm::cross(binormal, position - previous.position));
        addPoint({ position, normal, color });
        previous.normal = normal;
        return;
    }

    Point prevPrevious = points[points.size() - 2];
    // Set the normal of the previous point to the average of the normals of the previous two points
    // This is to make the curve smooth, when interpolating between the points
    previous.normal = glm::normalize(previous.normal + prevPrevious.normal);
}

void LinePath::addPoint(Point point)
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

Point LinePath::getPoint(int index) const
{
    return points.at(index);
}

size_t LinePath::getNumPoints()
{
    return points.size();
}

void LinePath::setPoint(int index, Point point)
{
    points.at(index) = point;
}

void LinePath::removePoint(int index)
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
