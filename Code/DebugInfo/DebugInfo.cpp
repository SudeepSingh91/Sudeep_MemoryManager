#include <stdarg.h>
#include <stdio.h>
#include <Windows.h>

namespace Debugging
{
	void OutputDebuggingInfo(const char * i_fmt, const char* i_file, const int i_lineNumber, ...)
	{
		const size_t len_temp = 256;
		char str_temp[len_temp] = { 0 };

		sprintf_s(str_temp, "DEBUG %s %i: ", i_file, i_lineNumber);
		strcat_s(str_temp, i_fmt);
		strcat_s(str_temp, "\n");

		const size_t len_output = len_temp + 32;
		char str_output[len_output] = { 0 };

		va_list args;
		va_start(args, i_lineNumber);
		vsprintf_s(str_output, len_output, str_temp, args);
		va_end(args);

		OutputDebugStringA(str_output);
	}

	void CheckCondition(const bool i_condition)
	{
		if (i_condition == false)
		{
			__debugbreak();
		}
	}
}