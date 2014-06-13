//-----------------------------------------------------------------------------------------------// 
// Utils.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_BASE_UTILS_H
#define MPX_BASE_UTILS_H

#include <string>

namespace mpx { namespace Base {

//-----------------------------------------------------------------------------------------------// 

void sprint(std::string& out, const char* pFormat, ...);

//-----------------------------------------------------------------------------------------------// 

std::string sprint(const char* pFormat, ...);

//-----------------------------------------------------------------------------------------------// 

}} // mpx::Base

//-----------------------------------------------------------------------------------------------// 

#endif
