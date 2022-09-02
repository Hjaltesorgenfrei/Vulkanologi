#include "Material.h"

Material Material::LoadFromPng(const char* filename) {
    Material material{};
    material.filename = filename;
    return material;
}