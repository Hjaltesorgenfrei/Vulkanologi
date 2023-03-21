#include "LinePath.h"

LinePath::LinePath(glm::vec3 start, glm::vec3 end, glm::vec3 color)
{
    glm::vec3 normal = glm::vec3(0, 1, 0);
    addPoint({ start, normal, color });
    addPoint({ end, normal, color  });
}

void LinePath::generateFrenetFrames()
{
    // TODO: Implement
}
