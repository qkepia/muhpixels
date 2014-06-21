//-----------------------------------------------------------------------------------------------// 
// Decode.cpp
//-----------------------------------------------------------------------------------------------// 

#include <BitStream.h>
#include <Decode.h>
#include <Utils.h>
#include <nestegg/include/nestegg/nestegg.h>
#include <stdarg.h>
#include <vpx/vp8dx.h>
#include <vpx/vpx_decoder.h>

#pragma warning (disable: 4996) // shut up safety warning

namespace mpx {

//-----------------------------------------------------------------------------------------------// 
// Decoder state data for decoding a single file
//-----------------------------------------------------------------------------------------------// 
class Decoder::State
{
public:
	std::unique_ptr<vpx_codec_ctx_t> pCodec;
	FILE* pFile = nullptr;
	nestegg* pNestegg = nullptr;
	nestegg_packet* pPacket = nullptr;
	uint nextChunk = 0;
	uint chunkCount = 0;
	uint videoTrackIdx = -1;
	vpx_image_t* pCurImage = nullptr;

	~State()
	{
		if(pNestegg)
			nestegg_destroy(pNestegg);		

		if(pFile)
			fclose(pFile);

		if(pCodec)
		{
			vpx_codec_destroy(pCodec.get()); 
			// todo: throw DecoderError(sprint("Failed to destroy decoder: %s", vpx_codec_error(pCodec)));
		}
	}
};

//-----------------------------------------------------------------------------------------------// 
// Nestegg callbacks
//-----------------------------------------------------------------------------------------------// 

int nesteggRead(void* pBuf, size_t length, void* pUserdata)
{
	FILE* pFile = static_cast<FILE*>(pUserdata);

	if(fread(pBuf, 1, length, pFile) < length) 
	{
		if(ferror(pFile))
			return -1;
		if(feof(pFile))
			return 0;
	}
	return 1;
}

//-----------------------------------------------------------------------------------------------// 

int nesteggSeek(int64_t offset, int origin, void *pUserdata) 
{
	switch(origin) 
	{
	case NESTEGG_SEEK_SET:
		origin = SEEK_SET;
		break;
	case NESTEGG_SEEK_CUR:
		origin = SEEK_CUR;
		break;
	case NESTEGG_SEEK_END:
		origin = SEEK_END;
		break;
	};

	return fseek(static_cast<FILE*>(pUserdata), long(offset), origin) ? -1 : 0;
}

//-----------------------------------------------------------------------------------------------// 

int64_t nesteggTell(void *pUserdata) 
{
	return ftell(static_cast<FILE*>(pUserdata));
}

//-----------------------------------------------------------------------------------------------// 

void nesteggLog(nestegg *pNestegg, uint severity, const char* pFormat, ...) 
{
	va_list vaList;
	va_start(vaList, pFormat);
	vfprintf(stderr, pFormat, vaList);
	fprintf(stderr, "\n");
	va_end(vaList);
}

//-----------------------------------------------------------------------------------------------// 

void Decoder::openFile(std::string file)
{	
	// (re-)initialize state
	m_pState = std::make_unique<State>();
	State& rState = *m_pState;
	
	// initialize decoder
	vpx_codec_dec_cfg_t config = { 0 };
	int flags = 0;
	rState.pCodec = std::make_unique<vpx_codec_ctx_t>();
	if(vpx_codec_dec_init(&*rState.pCodec, vpx_codec_vp9_dx(), &config, flags)) 
	{
		throw DecoderError(sprint("Failed to initialize decoder: %s", 
			vpx_codec_error(rState.pCodec.get())));
	}
	
	// try to open file
	const char* pFileName = file.c_str();
	if(!pFileName)
		throw DecoderError("Illegal Filename");

	rState.pFile = fopen(pFileName, "rb");
	if(!rState.pFile)
		throw DecoderError("Can't open file");
		
	// initialize nestegg library
	nestegg_io io = { nesteggRead, nesteggSeek, nesteggTell, nullptr };		
	io.userdata = rState.pFile;
	if(nestegg_init(&rState.pNestegg, io, nullptr))
		throw DecoderError("Nestegg error: init");

	// get number of tracks
	uint trackCount;
	if(nestegg_track_count(rState.pNestegg, &trackCount))
		throw DecoderError("Nestegg error: track count");

	// find the video track
	uint trackIdx = -1;
	for(uint i = 0; i < trackCount; i++) 
	{
		int trackType = nestegg_track_type(rState.pNestegg, i);
		if(trackType == NESTEGG_TRACK_VIDEO)
		{
			trackIdx = i;
			break;			
		}
		if(trackType < 0)
			throw DecoderError("Nestegg error: track type");
	}
	if(trackIdx == -1)
		throw DecoderError("No video track found");

	// get the codec for the video track, it should be VP9
	int codecId = nestegg_track_codec_id(rState.pNestegg, trackIdx);
	if(codecId != NESTEGG_CODEC_VP9) 
		throw DecoderError("Codec is not VP9");
	rState.videoTrackIdx = trackIdx;
}

//-----------------------------------------------------------------------------------------------// 

bool Decoder::decodeNextFrame()
{
	State& rState = *m_pState;
	rState.pCurImage = nullptr;
	
	// get buffer pointer and size for the next frame
	uint8_t* pBuf = nullptr;
	size_t bufSize = 0;
	{
		// todo: does this stuff actually work? my testcase only ever has one chunk
		if(rState.nextChunk >= rState.chunkCount) 
		{
			for(;;)
			{
				// end of packet, get another
				if(rState.pPacket)
					nestegg_free_packet(rState.pPacket);

				// read packet
				if(nestegg_read_packet(rState.pNestegg, &rState.pPacket) <= 0)
					return false; // 0 == end of stream, -1 == error

				uint trackIdx;
				if(nestegg_packet_track(rState.pPacket, &trackIdx) < 0)
					return false; // -1 == error

				if(trackIdx == rState.videoTrackIdx)
					break; // track found
			}

			// get number of chunks
			if(nestegg_packet_count(rState.pPacket, &rState.chunkCount))
				return false;

			rState.nextChunk = 0;
		}

		if(nestegg_packet_data(rState.pPacket, rState.nextChunk, &pBuf, &bufSize))
			return false;

		rState.nextChunk++;
	}

	// decode frame
	if(vpx_codec_decode(rState.pCodec.get(), pBuf, static_cast<uint>(bufSize), nullptr, 0)) 
	{
		std::string errorMsg = sprint("Failed to decode frame: %s", 
			vpx_codec_error(rState.pCodec.get()));
		const char* pDetail = vpx_codec_error_detail(rState.pCodec.get());
		if(pDetail)
			errorMsg += std::string(pDetail);
		throw DecoderError(errorMsg);
	}

	// get pointer to the image data buffer
	vpx_codec_iter_t codecIter = nullptr;
	rState.pCurImage = vpx_codec_get_frame(rState.pCodec.get(), &codecIter);
	
	// check for corruption
	int corrupted;
	if(vpx_codec_control(rState.pCodec.get(), VP8D_GET_FRAME_CORRUPTED, &corrupted)) 
	{
		throw DecoderError(sprint("Failed VP8_GET_FRAME_CORRUPTED: %s", 
			vpx_codec_error(rState.pCodec.get())));
	}

	return rState.pCurImage != nullptr;
}

//-----------------------------------------------------------------------------------------------// 

void Decoder::convertCurrentFrame(FrameBuf<RGB8>& rDestFrame) const
{
	State& rState = *m_pState;
	if(!rState.pCurImage)
		return; // no frame available

	// Convert from 4:2:0 subsampled YCbCr data to a buffer of RGB pixels.
	// No other formats are supported. (does VP9 even have others?)
	const vpx_image_t& srcImage = *rState.pCurImage;

	if(srcImage.fmt != VPX_IMG_FMT_I420)
		throw DecoderError("Unsupported image format. Only 4:2:0 allowed");

	uint c_w = srcImage.x_chroma_shift ? (1 + srcImage.d_w) >> srcImage.x_chroma_shift 
									   : srcImage.d_w;
	uint c_h = srcImage.y_chroma_shift ? (1 + srcImage.d_h) >> srcImage.y_chroma_shift 
									   : srcImage.d_h;

	if(c_w * 2 != srcImage.d_w || c_h * 2 != srcImage.d_h)
		throw DecoderError("Unsupported chroma subsampling format");

	uint32_t frameWidth = srcImage.d_w;
	uint32_t frameHeight = srcImage.d_h;
	rDestFrame.setSize(frameWidth, frameHeight);

	int strideY = srcImage.stride[VPX_PLANE_Y];
	int strideU = srcImage.stride[VPX_PLANE_U];
	int strideV = srcImage.stride[VPX_PLANE_V];

	for(uint32_t j = 0; j < frameHeight/2; j++)
	{
		const uint8_t* pY0 = srcImage.planes[VPX_PLANE_Y] + 2 * j * strideY;
		const uint8_t* pY1 = pY0 + strideY;
		const uint8_t* pU = srcImage.planes[VPX_PLANE_U] + j * strideU;
		const uint8_t* pV = srcImage.planes[VPX_PLANE_V] + j * strideV;

		for(uint32_t i = 0; i < frameWidth/2; i++)
		{
			uint8_t ys[4];
			ys[0] = *pY0++;
			ys[1] = *pY0++;
			ys[2] = *pY1++;
			ys[3] = *pY1++;
			uint8_t u = *pU++;
			uint8_t v = *pV++;

			YUV8 yuv0 = { ys[0], u, v };
			YUV8 yuv1 = { ys[1], u, v };
			YUV8 yuv2 = { ys[2], u, v };
			YUV8 yuv3 = { ys[3], u, v };
			RGB8 rgb0 = toRGB(yuv0);
			RGB8 rgb1 = toRGB(yuv1);
			RGB8 rgb2 = toRGB(yuv2);
			RGB8 rgb3 = toRGB(yuv3);
			rDestFrame(2*i, 2*j) = rgb0;
			rDestFrame(2*i+1, 2*j) = rgb1;
			rDestFrame(2*i, 2*j+1) = rgb2;
			rDestFrame(2*i+1, 2*j+1) = rgb3;
		}
	}
}

//-----------------------------------------------------------------------------------------------// 

void modelBitStream(std::string file, 
					BitStreamInfo& info,
					FrameBuf<RGB8>& firstFrame)
{
	Decoder decoder;
	decoder.openFile(file);
	int iFrame = 0;
	for(; decoder.decodeNextFrame(); iFrame++)
	{
		if(firstFrame.size() == 0)
		{
			decoder.convertCurrentFrame(firstFrame);
		}
	}
}

//-----------------------------------------------------------------------------------------------// 

} // mpx