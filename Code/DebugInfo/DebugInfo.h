#pragma once

namespace Debugging
{
	void OutputDebuggingInfo(const char * i_fmt, const char* file, const int line_number, ...);
	void CheckCondition(const bool i_condition);
}

#if defined(_DEBUG)
#define DEBUG_PRINT(fmt,...) Debugging::OutputDebuggingInfo((fmt), __FILE__, __LINE__, __VA_ARGS__)
#define assert(condition) Debugging::CheckCondition(condition)
#else
#define DEBUG_PRINT(fmt,...) void(0)
#define assert(condition) void(0)
#endif