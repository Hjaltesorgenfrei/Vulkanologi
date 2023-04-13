#pragma once

#include <glm/glm.hpp>

struct Color {
    // Red, Blue, Teal, Purple, Yellow, Orange, Green, Pink, Grey, Light Blue, Dark Green, Brown


    static glm::vec3 random() {
        return glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
    }

    static glm::vec3 randomBright() {
        return glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX) * 0.5f + 0.5f;
    }

    static glm::vec3 randomPastel() {
        return glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX) * 0.5f + 0.25f;
    }

    static glm::vec3 randomDark() {
        return glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX) * 0.5f;
    }

    // const glm::vec3 RED = glm::vec3(1.0f, 0.0f, 0.0f);
    // const glm::vec3 BLUE = glm::vec3(0.0f, 0.0f, 1.0f);  
    // const glm::vec3 TEAL = glm::vec3(0.0f, 1.0f, 1.0f);
    // const glm::vec3 PURPLE = glm::vec3(1.0f, 0.0f, 1.0f);
    // const glm::vec3 YELLOW = glm::vec3(1.0f, 1.0f, 0.0f);
    // const glm::vec3 ORANGE = glm::vec3(1.0f, 0.5f, 0.0f);
    // const glm::vec3 GREEN = glm::vec3(0.0f, 1.0f, 0.0f);
    // const glm::vec3 PINK = glm::vec3(1.0f, 0.0f, 0.5f);
    // const glm::vec3 GREY = glm::vec3(0.5f, 0.5f, 0.5f);
    // const glm::vec3 LIGHT_BLUE = glm::vec3(0.5f, 0.5f, 1.0f);
    // const glm::vec3 DARK_GREEN = glm::vec3(0.0f, 0.5f, 0.0f);
    // const glm::vec3 BROWN = glm::vec3(0.5f, 0.25f, 0.0f);
    // const glm::vec3 WHITE = glm::vec3(1.0f, 1.0f, 1.0f);
    // const glm::vec3 BLACK = glm::vec3(0.0f, 0.0f, 0.0f);

    static glm::vec3 playerColor(int i) {
        switch(i) {
            case 0: return glm::vec3(1.0f, 0.0f, 0.0f);
            case 1: return glm::vec3(0.0f, 0.0f, 1.0f);
            case 2: return glm::vec3(0.0f, 1.0f, 1.0f);
            case 3: return glm::vec3(1.0f, 0.0f, 1.0f);
            case 4: return glm::vec3(1.0f, 1.0f, 0.0f);
            case 5: return glm::vec3(1.0f, 0.5f, 0.0f);
            case 6: return glm::vec3(0.0f, 1.0f, 0.0f);
            case 7: return glm::vec3(1.0f, 0.0f, 0.5f);
            case 8: return glm::vec3(0.5f, 0.5f, 0.5f);
            case 9: return glm::vec3(0.5f, 0.5f, 1.0f);
            case 10: return glm::vec3(0.0f, 0.5f, 0.0f);
            case 11: return glm::vec3(0.5f, 0.25f, 0.0f);
            default: return glm::vec3(1.0f, 1.0f, 1.0f);
        }
    }
};


