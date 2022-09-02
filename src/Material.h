#pragma once

struct Material {
	const char* filename;

	static Material LoadFromPng(const char* filename);
};