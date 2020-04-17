// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <cstdio>
#include <string>
#include <fstream>
#include <ctime>

#ifdef WIN32
	#include <sys\timeb.h>
#else
	#include <sys/time.h>
#endif

#include <direct.h>

#ifdef WIN32
	#include "include\dirent.h"
#else
	#include <dirent.h>
#endif

#include <regex>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <map>
