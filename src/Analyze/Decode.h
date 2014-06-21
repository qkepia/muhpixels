//-----------------------------------------------------------------------------------------------// 
// Decode.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_ANALYZE_DECODE_H
#define MPX_ANALYZE_DECODE_H

#include <string>
#include <FrameBuf.h>
#include <Color.h>
#include <memory>

#include <vpx/vpx_decoder.h>
#include "nestegg/include/nestegg/nestegg.h"

namespace mpx {

struct BitStreamInfo;

//-----------------------------------------------------------------------------------------------// 

class Decoder
{
public:
	void openFile(std::string file);
	vpx_image_t* getNextFrame();
	void convertFrame(FrameBuf<RGB8>& rDestFrame, const vpx_image_t& rSrcImage);

private:
	std::shared_ptr<nestegg> m_pNestegg;
	std::shared_ptr<vpx_codec_ctx_t> m_pCodecContext;
};

//-----------------------------------------------------------------------------------------------// 

class DecoderError : std::exception 
{
public:
	DecoderError() {}
	DecoderError(std::string error) : m_error(error) {}
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
