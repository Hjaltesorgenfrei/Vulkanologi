#include "Model.h"
#include <chrono>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

const std::string MODEL_PATH = "resources/viking_room.obj";



namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
	};
}

Model::Model() {
    loadModel();

    cameraYaw   = -135.0f;
    cameraPitch = -35.0f;
    cameraPosition = glm::vec3(2.0f, 2.0f, 2.0f);
    cameraUp       = glm::vec3(0.0f, 0.0f, 1.0f);
    setCameraFront();
}

void Model::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error("Model failed to load!\n" + warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{
                .pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                },
                .color = {1.0f, 1.0f, 1.0f},
                .texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                }
            };

            if(uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            
            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

const std::vector<Vertex> Model::getVertices() {
    return vertices;
}
const std::vector<uint32_t> Model::getIndices() {
    return indices;
}

const MeshPushConstants Model::getPushConstants() {
    return {
            .model = {0.0f, 0.0f, 0.0f, 1.0f}
    };
}

const UniformBufferObject Model::getCameraProject(float width, float height) {
    glm::vec3 cameraDirection = glm::normalize(cameraPosition - glm::vec3(0.0f, 0.0f, 0.0f));
	UniformBufferObject ubo {
		.view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp),
		.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 1000.0f)
	};

	ubo.proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

    return ubo;
}

void Model::moveCameraForward(float speed) {
    cameraPosition += speed * cameraFront;
}

void Model::moveCameraBackward(float speed) {
    cameraPosition -= speed * cameraFront;
}

void Model::moveCameraLeft(float speed) {
    cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void Model::moveCameraRight(float speed) {
    cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void Model::newCursorPos(float xPos, float yPos) {
    if (firstCursorCall)
    {
        cursorXPos = xPos;
        cursorYPos = yPos;
        firstCursorCall = false;
        return;
    }

    float xOffset = cursorXPos - xPos;
    float yOffset = cursorYPos - yPos; // reversed since y-coordinates go from bottom to top
    cursorXPos = xPos;
    cursorYPos = yPos;

    float sensitivity = 0.2f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    cameraYaw += xOffset;
    cameraPitch += yOffset;

    // make sure that when cameraPitch is out of bounds, screen doesn't get flipped
    if (cameraPitch > 89.0f)
        cameraPitch = 89.0f;
    if (cameraPitch < -89.0f)
        cameraPitch = -89.0f;
        
    setCameraFront();
}

void Model::setCameraFront() {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraPitch));
    cameraFront = glm::normalize(front);
}

void Model::resetCursorPos() {
    firstCursorCall = true;
}

