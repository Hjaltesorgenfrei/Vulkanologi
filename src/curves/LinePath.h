#pragma once

#ifndef LINEPATH_H
#define LINEPATH_H

#include "Path.h"

class LinePath : public Path {
public:
    LinePath(glm::vec3 start, glm::vec3 end, glm::vec3 color);

    void generateFrenetFrames() override;
};

#endif // LINEPATH_H