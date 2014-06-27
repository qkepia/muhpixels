//-----------------------------------------------------------------------------------------------// 
// Decode.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_ANALYZE_DECODE_H
#define MPX_ANALYZE_DECODE_H

#include <Color.h>
#include <FrameBuf.h>
#include <memory>
#include <string>

namespace mpx {

//-----------------------------------------------------------------------------------------------// 

class Decoder
{
public:
	void openFile(std::string file);
	bool decodeNextFrame();
	void convertCurrentFrame(FrameBuf<RGB8>& rDestFrame) const;

private:
	class State;
	std::unique_ptr<State> m_pState; // pimpl for the decoder state
};

//-----------------------------------------------------------------------------------------------// 

class DecoderError : public std::exception 
{
public:
	DecoderError() {}
	DecoderError(std::string error) : m_error(error) {}
	const char* what() const { return m_error.c_str(); }
private:
	std::string m_error;
};

//-----------------------------------------------------------------------------------------------// 

struct BitStreamInfo;

void modelBitStream(std::string file, 
					BitStream& info,
					FrameBuf<RGB8>& firstFrame);

//-----------------------------------------------------------------------------------------------// 

} // mpx

//-----------------------------------------------------------------------------------------------// 

#endif
