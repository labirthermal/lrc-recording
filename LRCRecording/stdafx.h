// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef WIN32
	#include "targetver.h"
#endif

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
