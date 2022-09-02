#pragma once
#include "Mesh.h"
#include "Material.h"

class Model{
public: 
    Model(Mesh mesh, Material material) {
        this->mesh = mesh;
        this->material = material;
    }
    Mesh mesh;
    Material material;
};
