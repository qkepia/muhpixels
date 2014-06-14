//-----------------------------------------------------------------------------------------------// 
// Decode.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_ANALYZE_DECODE_H
#define MPX_ANALYZE_DECODE_H

#include <string>
#include <FrameBuf.h>
#include <Color.h>

namespace mpx {

struct BitStreamInfo;

//-----------------------------------------------------------------------------------------------// 

class DecodeError : std::exception 
{
public:
	DecodeError() {}
	DecodeError(std::string error) : m_error(error) {}
	const char* what() const { return m_error.c_str(); }
private:
	std::string m_error;
};

//-----------------------------------------------------------------------------------------------// 

void modelBitStream(std::string file, 
					BitStreamInfo& info,
					FrameBuf<RGB8>& firstFrame);

//-----------------------------------------------------------------------------------------------// 

} // mpx

//-----------------------------------------------------------------------------------------------// 

#endif
