#include "PhysicsDebugDrawer.hpp"

PhysicsDebugDrawer::PhysicsDebugDrawer() {
	Initialize();
}

// TODO: Implement these functions in a efficient manner
// DebugRendererRecorder has some implementation i can probably steal quite a bit of.
void PhysicsDebugDrawer::DrawLine(RVec3Arg inFrom, RVec3Arg inTo, ColorArg inColor) {
	_lines.emplace_back(glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()),
						glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ()));
}

void PhysicsDebugDrawer::DrawTriangle(RVec3Arg inV1, RVec3Arg inV2, RVec3Arg inV3, ColorArg inColor,
									  ECastShadow inCastShadow) {
	triangle++;
}

DebugRenderer::Batch PhysicsDebugDrawer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) {
	triangle += inTriangleCount;
	return Batch();
}

DebugRenderer::Batch PhysicsDebugDrawer::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount,
															 const uint32* inIndices, int inIndexCount) {
	triangle += inIndexCount / 3;
	return Batch();
}

void PhysicsDebugDrawer::DrawGeometry(RMat44Arg inModelMatrix, const AABox& inWorldSpaceBounds, float inLODScaleSq,
									  ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode,
									  ECastShadow inCastShadow, EDrawMode inDrawMode) {
	geometry++;
}

void PhysicsDebugDrawer::DrawText3D(RVec3Arg inPosition, const string_view& inString, ColorArg inColor,
									float inHeight) {
	text++;
}

std::vector<std::pair<glm::vec3, glm::vec3>> PhysicsDebugDrawer::lines() {
	return _lines;
}

void PhysicsDebugDrawer::clear() {
	triangle = 0;
	geometry = 0;
	text = 0;
	_lines.clear();
}
