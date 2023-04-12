#pragma once

#ifndef LINEPATH_H
#define LINEPATH_H

#include "Path.hpp"

class LinePath : public Path {
public:
    LinePath(glm::vec3 start, glm::vec3 end, glm::vec3 color);

    void addPoint(glm::vec3 position, glm::vec3 color);
    void addPoint(Point point);
    Point getPoint(int index) const;
    size_t getNumPoints();
    void setPoint(int index, Point point);
    void removePoint(int index);
};

#endif // LINEPATH_H