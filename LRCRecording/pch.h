// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here

#include "LRCRecordingAPI.h"

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
//#include <windows.h>

#include <cstdio>
#include <string>
#include <fstream>
#include <ctime>
#include <cfloat>
#include <cmath>
#include <sys/stat.h>

#ifdef WIN32
	#include <sys/timeb.h>
#else
	#include <sys/time.h>
#endif

#ifdef WIN32
	#include <direct.h>
	#include "include/dirent.h"
#else
	#include <dirent.h>
#endif

#include <regex>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <map>

#endif //PCH_H
