#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Math/Real.h>
#include <glm/glm.hpp>
/*
Physics.cpp(301, 22): pure virtual function "JPH::DebugRenderer::DrawTriangle" has no overrider
Physics.cpp(301, 22): pure virtual function "JPH::DebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle
*inTriangles, int inTriangleCount)" has no overrider Physics.cpp(301, 22): pure virtual function
"JPH::DebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Vertex *inVertices, int inVertexCount, const
JPH::uint32 *inIndices, int inIndexCount)" has no overrider Physics.cpp(301, 22): pure virtual function
"JPH::DebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, float
inLODScaleSq, JPH::ColorArg inModelColor, const JPH::DebugRenderer::GeometryRef &inGeometry,
JPH::DebugRenderer::ECullMode inCullMode = JPH::DebugRenderer::ECullMode::CullBackFace, JPH::DebugRenderer::ECastShadow
inCastShadow = JPH::DebugRenderer::ECastShadow::On, JPH::DebugRenderer::EDrawMode inDrawMode =
JPH::DebugRenderer::EDrawMode::Solid)" has no overrider Physics.cpp(301, 22): pure virtual function
"JPH::DebugRenderer::DrawText3D" has no overrider
*/

using namespace JPH;

class PhysicsDebugDrawer : public DebugRenderer {
	std::vector<std::pair<glm::vec3, glm::vec3>> _lines;
	int geometry = 0, triangle = 0, text = 0;

public:
	PhysicsDebugDrawer();
	void DrawLine(RVec3Arg inFrom, RVec3Arg inTo, ColorArg inColor) override;
	void DrawTriangle(RVec3Arg inV1, RVec3Arg inV2, RVec3Arg inV3, ColorArg inColor,
					  ECastShadow inCastShadow = ECastShadow::Off) override;
	Batch CreateTriangleBatch(const Triangle *inTriangles, int inTriangleCount);
	Batch CreateTriangleBatch(const Vertex *inVertices, int inVertexCount, const uint32 *inIndices, int inIndexCount);
	void DrawGeometry(RMat44Arg inModelMatrix, const AABox &inWorldSpaceBounds, float inLODScaleSq,
					  ColorArg inModelColor, const GeometryRef &inGeometry,
					  ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::On,
					  EDrawMode inDrawMode = EDrawMode::Solid);
	void DrawText3D(RVec3Arg inPosition, const string_view &inString, ColorArg inColor = Color::sWhite,
					float inHeight = 0.5f);
	std::vector<std::pair<glm::vec3, glm::vec3>> lines();
	void clear();
};