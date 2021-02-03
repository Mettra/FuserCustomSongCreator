#pragma once

#ifndef _OV_FILE_H_
#include "XiphTypes.h"
#endif

// Callbacks using standard C FILE* as a datasource.
extern ov_callbacks cCallbacks;
// Callbacks using a C++ ifstream* as a datasource.
extern ov_callbacks cppCallbacks;