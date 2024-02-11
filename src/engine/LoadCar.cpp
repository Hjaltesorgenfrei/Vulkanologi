#include "LoadCar.hpp"

#include <tiny_obj_loader.h>

#include <glm/gtx/hash.hpp>
#include <iostream>
#include <string>
#include "Components.hpp"

void LoadCar(const char *filename, CarSettings &settings) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	// Get path from filename
	std::string fileDir = filename;
	size_t lastSlash = fileDir.find_last_of("/\\");
	if (lastSlash != std::string::npos) {
		fileDir = fileDir.substr(0, lastSlash + 1);
	} else {
		fileDir = "./";
	}

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, fileDir.c_str())) {
		throw std::runtime_error("Car failed to load!\n" + warn + err);
	}

	if (!warn.empty()) {
		std::cout << "WARN: " << warn << " File: " << filename << std::endl;
	}
	if (!err.empty()) {
		std::cerr << "ERR: " << err << " File: " << filename << std::endl;
	}

	glm::vec3 carCenter(0.f), backLeft(0.f), backRight(0.f), frontLeft(0.f), frontRight(0.f);
	// Loop over shapes
	for (auto &shape : shapes) {
		// Loop over faces(polygon)
		float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
		float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			auto fv = size_t(shape.mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t index = shape.mesh.indices[index_offset + v];

				minX = std::min(minX, attrib.vertices[3 * index.vertex_index + 0]);
				minY = std::min(minY, attrib.vertices[3 * index.vertex_index + 1]);
				minZ = std::min(minZ, attrib.vertices[3 * index.vertex_index + 2]);
				maxX = std::max(maxX, attrib.vertices[3 * index.vertex_index + 0]);
				maxY = std::max(maxY, attrib.vertices[3 * index.vertex_index + 1]);
				maxZ = std::max(maxZ, attrib.vertices[3 * index.vertex_index + 2]);
			}
			index_offset += fv;
		}
		glm::vec3 center(minX + (maxX - minX) / 2, minY + (maxY - minY) / 2, minZ + (maxZ - minZ) / 2);
		if (shape.name.starts_with("body")) {
			carCenter = center;
		} else if (shape.name.ends_with("BackLeft")) {
			backLeft = center;
		} else if (shape.name.ends_with("BackRight")) {
			backRight = center;
		} else if (shape.name.ends_with("FrontLeft")) {
			settings.wheelRadius = (maxY - minY) / 2;
			settings.wheelWidth = (minX - maxX);
			frontLeft = center;
		} else if (shape.name.ends_with("FrontRight")) {
			frontRight = center;
		}
	}
	settings.frontWheelOffset.Width = glm::distance(frontLeft, frontRight) / 2;
	settings.rearWheelOffset.Width = glm::distance(backLeft, backRight) / 2;
	settings.frontWheelOffset.Length = frontLeft.z;
	settings.frontWheelOffset.Height = frontLeft.y;
	settings.rearWheelOffset.Length = backLeft.z;
	settings.rearWheelOffset.Height = backLeft.y;
}
