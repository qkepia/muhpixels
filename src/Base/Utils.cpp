//-----------------------------------------------------------------------------------------------// 
// Utils.cpp
//-----------------------------------------------------------------------------------------------// 

#include <Utils.h>
#include <stdarg.h>
#include <memory>
#include <cstdlib>

#pragma warning (disable: 4996) // shut up about strcpy

namespace mpx {

//-----------------------------------------------------------------------------------------------// 
// printf to std::string
//-----------------------------------------------------------------------------------------------// 
void sprint(std::string& out, const char* pFormat, ...)
{
	// start with a buffer twice the size of the format string
	int lenFormat = static_cast<int>(strlen(pFormat));
	int lenCurrent = 2 * lenFormat;

	// try until it fits
    std::unique_ptr<char[]> formatted;
    va_list argList;
    for(;;)
	{
		// try with the current length
        formatted.reset(new char[lenCurrent]);
        strcpy(&formatted[0], pFormat);
        va_start(argList, pFormat);
        int lenFinal = vsnprintf(&formatted[0], lenCurrent, pFormat, argList);
        va_end(argList);

		if(lenFinal > 0 && lenFinal < lenCurrent)
		{
			// it fit
			break;
		}
		else
		{
			// it didn't
            lenCurrent += abs(lenFinal - lenCurrent + 1);
		}
    }

	// copy to output
	out = std::string(formatted.get());
}

//-----------------------------------------------------------------------------------------------// 

std::string sprint(const char* pFormat, ...)
{
	va_list argList; 
	va_start(argList, pFormat); 
	
	std::string out;
	sprint(out, pFormat);
	return out;
}

//-----------------------------------------------------------------------------------------------// 

} // mpx