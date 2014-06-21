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
	class Error : public std::exception 
	{
	public:
		Error() {}
		Error(std::string error) : m_error(error) {}
		const char* what() const { return m_error.c_str(); }
	private:
		std::string m_error;
	};
	
	Decoder();
	~Decoder();

	void openFile(std::string file);
	bool decodeNextFrame();
	void convertCurrentFrame(FrameBuf<RGB8>& rDestFrame) const;

private:
	struct VPX;
	std::unique_ptr<VPX> m_pVPX; // VPX codec pimpl
	FILE* m_pFile = nullptr;
};

//-----------------------------------------------------------------------------------------------// 

struct BitStreamInfo;

void modelBitStream(std::string file, 
					BitStreamInfo& info,
					FrameBuf<RGB8>& firstFrame);

//-----------------------------------------------------------------------------------------------// 

} // mpx

//-----------------------------------------------------------------------------------------------// 

#endif
