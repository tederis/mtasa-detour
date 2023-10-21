#pragma once

#include <cstdint>

#ifdef EXPORT_NATIVE_API

#ifndef NAVIGATION_API
	#ifdef NAVIGATION_EXPORT
		// We are building this library
		#ifdef WIN32			
			#define NAVIGATION_API __declspec(dllexport)
		#else
			#define NAVIGATION_API __attribute__((visibility("default")))
		#endif
	#else
		// We are using this library
		#ifdef WIN32			
			#define NAVIGATION_API __declspec(dllimport)
		#else

		#endif
	#endif
#endif

extern "C"
{
	bool NAVIGATION_API navInit();

	void NAVIGATION_API navShutdown();

	bool NAVIGATION_API navState();

	bool NAVIGATION_API navLoad(const char* filename);

	bool NAVIGATION_API navSave(const char* filename);

	bool NAVIGATION_API navFindPath(float* startPos, float* endPos, int* outPointsNum, float* outPoints);

	bool NAVIGATION_API navNearestPoint(float* startPos, float* outPoint);

	bool NAVIGATION_API navDump(const char* filename);

	bool NAVIGATION_API navBuild();

	bool NAVIGATION_API navCollisionMesh(float* boundsMin, float* boundsMax, float bias, int* outVerticesNum, float* outVertices);

	bool NAVIGATION_API navNavigationMesh(float* boundsMin, float* boundsMax, float bias, int* outVerticesNum, float* outVertices);

	bool NAVIGATION_API navScanWorld(float* boundsMin, float* boundsMax, int* outModelsNum, int* outModels);
}

#endif // EXPORT_NATIVE_API