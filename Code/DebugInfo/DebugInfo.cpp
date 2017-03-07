#include <stdarg.h>
#include <stdio.h>
#include <Windows.h>

namespace Debugging
{
	void OutputDebuggingInfo(const char* format, const char* file, const int line_number, ...)
	{
		const size_t len_temp = 256;
		char str_temp[len_temp] = { 0 };

		sprintf_s(str_temp, "DEBUG %s %i: ", file, line_number);
		strcat_s(str_temp, format);
		strcat_s(str_temp, "\n");

		const size_t len_output = len_temp + 32;
		char str_output[len_output] = { 0 };

		va_list args;
		va_start(args, line_number);
		vsprintf_s(str_output, len_output, str_temp, args);
		va_end(args);

		OutputDebugStringA(str_output);
	}

	void CheckCondition(const bool i_condition)
	{
		if (i_condition == false)
			__debugbreak();
	}
}