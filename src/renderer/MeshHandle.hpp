#pragma once
#include <cstdint>

class MeshHandle {
public:
	MeshHandle();
	uint64_t id;

	bool operator==(const MeshHandle& other) const { return id == other.id; }
};

template <>
struct std::hash<MeshHandle> {
	std::size_t operator()(const MeshHandle& h) const { return std::hash<uint64_t>()(h.id); }
};