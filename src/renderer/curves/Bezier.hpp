#pragma once

#ifndef BEZIER_H
#define BEZIER_H

#include <memory>

#include "Path.hpp"

class ControlPoint {
public:
	Point point() const {
		Point newPoint;
		newPoint.position = glm::vec3(transform * glm::vec4(_point.position, 1.0f));
		newPoint.normal = glm::vec3(transform * glm::vec4(_point.normal, 0.0f));
		newPoint.color = _point.color;
		return newPoint;
	}

	void setPoint(Point point) { _point = point; }

	glm::vec3 forward() const { return glm::vec3(transform * glm::vec4(_forward, 0.0f)); }

	glm::vec3 forwardWorld() const { return point().position + forward(); }

	glm::vec3 backwardWorld() const { return point().position - forward(); }

	void setForward(glm::vec3 forward) { _forward = forward; }

	void update(glm::mat4 transform) {
		if (transform != this->transform) {
			this->transform = transform;
			dirty = true;
		}
	}

	glm::mat4 transform = glm::mat4(1.0f);

	friend class Bezier;

private:
	Point _point = {glm::vec3(0.0f), glm::vec3(0, 1, 0), glm::vec3(1.0f)};
	glm::vec3 _forward = {1.0f, 0.f, 0.f};  // Is mirrored if the points is not an end point
	bool dirty = true;  // Might want a pointer to the bezier instead, as this currently needs all points to be checked.
};

class Bezier : public Path {
public:
	Bezier(glm::vec3 color);
	Bezier(std::vector<std::shared_ptr<ControlPoint>> controlPoints, glm::vec3 color);

	void recompute() override;
	bool recomputeIfDirty();

	void addPoint(std::shared_ptr<ControlPoint> controlPoint);
	std::shared_ptr<ControlPoint> getPoint(int index) const;
	size_t getNumPoints();
	void setPoint(int index, std::shared_ptr<ControlPoint> controlPoint);
	void removePoint(int index);
	std::vector<std::shared_ptr<ControlPoint>> const &getControlPoints() const;
	FrenetFrame frameAt(float t) const;

protected:
	std::vector<std::shared_ptr<ControlPoint>> controlPoints;
	glm::vec3 color;
	std::vector<std::pair<float, FrenetFrame>> sortedFrames;

	std::vector<glm::vec3> evenPointsAlongCubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
													 std::vector<float> &evenTs);

	std::vector<float> evenTsAlongCubic(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);

	std::vector<std::pair<float, float>> arcLength(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);

	float estimateArcLength(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);

	std::vector<FrenetFrame> getRMFrames(Point a, glm::vec3 b, glm::vec3 c, glm::vec3 d, std::vector<float> &evenTs);
};

/* TODO: Add this
glm::mat4 moveAlongCubicPath(ControlPoint a, ControlPoint b, int segments, int resolution, float t) {
	auto frames = generateRMFrames(a.point(), a.forwardWorld(), b.backwardWorld(), b.point().position, segments,
resolution); auto frame = frames[(int)(t * (float)frames.size()) % frames.size()]; return frenetFrameMatrix(frame);
}
*/

#endif  // BEZIER_H