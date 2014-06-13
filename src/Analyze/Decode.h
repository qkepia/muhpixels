//-----------------------------------------------------------------------------------------------// 
// Decode.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_ANALYZE_DECODE_H
#define MPX_ANALYZE_DECODE_H

#include <string>
#include <Model/BitStream.h>

namespace mpx { namespace Analyze {

//-----------------------------------------------------------------------------------------------// 

void modelBitStream(std::string file, Model::BitStreamInfo& info);

//-----------------------------------------------------------------------------------------------// 

}} // mpx::Analyze

//-----------------------------------------------------------------------------------------------// 

#endif
