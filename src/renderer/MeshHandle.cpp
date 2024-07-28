#include "MeshHandle.hpp"
#include <atomic>

uint64_t generateIdGlobalAtomic() {
	static std::atomic<uint64_t> globalCounter(0);
	return ++globalCounter;
}

MeshHandle::MeshHandle() {
	id = generateIdGlobalAtomic();
}
