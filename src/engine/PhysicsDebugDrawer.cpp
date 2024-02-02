#include "PhysicsDebugDrawer.hpp"

PhysicsDebugDrawer::PhysicsDebugDrawer() {
	Initialize();
}

// TODO: Implement these functions in a efficient manner
void PhysicsDebugDrawer::DrawLine(RVec3Arg inFrom, RVec3Arg inTo, ColorArg inColor) {}

void PhysicsDebugDrawer::DrawTriangle(RVec3Arg inV1, RVec3Arg inV2, RVec3Arg inV3, ColorArg inColor,
									  ECastShadow inCastShadow) {}

DebugRenderer::Batch PhysicsDebugDrawer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) {
	return Batch();
}

DebugRenderer::Batch PhysicsDebugDrawer::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount,
															 const uint32* inIndices, int inIndexCount) {
	return Batch();
}

void PhysicsDebugDrawer::DrawGeometry(RMat44Arg inModelMatrix, const AABox& inWorldSpaceBounds, float inLODScaleSq,
									  ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode,
									  ECastShadow inCastShadow, EDrawMode inDrawMode) {}

void PhysicsDebugDrawer::DrawText3D(RVec3Arg inPosition, const string_view& inString, ColorArg inColor,
									float inHeight) {}
