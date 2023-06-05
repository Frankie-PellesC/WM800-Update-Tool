#ifndef UPDATE_TOOL_H__
#define UPDATE_TOOL_H__
#pragma once

#include <stdint.h>
#include "main.h"

#define NELEMS(a)  (sizeof(a) / sizeof((a)[0]))

#define RET_SUCCESS				 0
#define RET_ERROR				-1

#define UPDATE_TOOL_VERSION		"1.00.10"

extern HINSTANCE ghInstance;

#endif	// UPDATE_TOOL_H__
