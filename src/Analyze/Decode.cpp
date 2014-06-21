//-----------------------------------------------------------------------------------------------// 
// Decode.cpp
//-----------------------------------------------------------------------------------------------// 

#include <BitStream.h>
#include <Decode.h>
#include <NesteggUtils.h>
#include <Utils.h>
#include <nestegg/include/nestegg/nestegg.h>
#include <vpx/vp8dx.h>
#include <vpx/vpx_decoder.h>

#define VP9_FOURCC_MASK (0x00395056)

#pragma warning (disable: 4996) // shut up safety warning

namespace mpx {

//-----------------------------------------------------------------------------------------------// 

struct Decoder::VPX
{
	nestegg* pNestegg = nullptr;
	vpx_codec_ctx_t codecCtx;
	vpx_image_t* pCurrentFrame = nullptr;
	input_ctx inputCtx;

	~VPX()
	{
		// shutdown codec and nestegg stream handle
		if(vpx_codec_destroy(&codecCtx)) 
			throw Error(sprint("Failed to destroy decoder: %s", vpx_codec_error(&codecCtx)));

		if(pNestegg)
			nestegg_destroy(pNestegg);
	}
};

//-----------------------------------------------------------------------------------------------// 

Decoder::Decoder()
	: m_pVPX(std::make_unique<VPX>())
{
}

//-----------------------------------------------------------------------------------------------// 

Decoder::~Decoder()
{
	if(m_pFile)
	{
		fclose(m_pFile);
		m_pFile = nullptr;
	}
}

//-----------------------------------------------------------------------------------------------// 

void Decoder::openFile(std::string file)
{
	// close current file, if any, and try to open new one
	if(m_pFile)
	{
		fclose(m_pFile);
		m_pFile = nullptr;
	}
	
	const char* pFileName = file.c_str();
	if(!pFileName)
		throw Error("Illegal Filename");

	m_pFile = fopen(pFileName, "rb");
	if(!m_pFile)
		throw Error("Can't open file");
	
	// fill input context structure
	memset(&m_pVPX->inputCtx, 0, sizeof(input_ctx));
		
	// read first file info stuff
	m_pVPX->inputCtx.infile = m_pFile;
	unsigned int fourcc, width, height, fps_den, fps_num;
	if(file_is_webm(&m_pVPX->inputCtx, &fourcc, &width, &height, &fps_den, &fps_num))
	{
		m_pVPX->inputCtx.kind = WEBM_FILE;
	}
	else
	{
		throw Error("Unrecognized input file type");
	}

	// check fourcc
	vpx_codec_iface_t* pCodecInterface = nullptr;
	if((fourcc & 0x00FFFFFF) !=  VP9_FOURCC_MASK)
		throw Error("Only VP9 supported");	
	
	pCodecInterface = vpx_codec_vp9_dx();

	// initialize decoder
	vpx_codec_dec_cfg_t cfg = { 0 };
	int dec_flags = 0;	
	if(vpx_codec_dec_init(&m_pVPX->codecCtx, pCodecInterface, &cfg, dec_flags)) 
	{
		throw Error(sprint("Failed to initialize decoder: %s", 
			vpx_codec_error(&m_pVPX->codecCtx)));
	}
}

//-----------------------------------------------------------------------------------------------// 

bool Decoder::decodeNextFrame()
{
	m_pVPX->pCurrentFrame = nullptr;
	
	uint8_t* buf = nullptr;
	size_t buf_sz = 0;
	size_t buf_alloc_sz = 0;
	if(read_frame(&m_pVPX->inputCtx, &buf, &buf_sz, &buf_alloc_sz)) 
	{
		// todo: is it ok to return early here?
		return false;
	}

	// decode frame
	if(vpx_codec_decode(&m_pVPX->codecCtx, buf, (unsigned int)buf_sz, nullptr, 0)) 
	{
		std::string errorMsg = sprint("Failed to decode frame: %s", 
			vpx_codec_error(&m_pVPX->codecCtx));
		const char* pDetail = vpx_codec_error_detail(&m_pVPX->codecCtx);
		if(pDetail)
			errorMsg += std::string(pDetail);
		throw Error(errorMsg);
	}

	vpx_codec_iter_t codecIter = nullptr;
	m_pVPX->pCurrentFrame = vpx_codec_get_frame(&m_pVPX->codecCtx, &codecIter);
	
	// check for corruption
	int corrupted;
	if(vpx_codec_control(&m_pVPX->codecCtx, VP8D_GET_FRAME_CORRUPTED, &corrupted)) 
	{
		throw Error(sprint("Failed VP8_GET_FRAME_CORRUPTED: %s", 
			vpx_codec_error(&m_pVPX->codecCtx)));
	}

	return m_pVPX->pCurrentFrame != nullptr;
}

//-----------------------------------------------------------------------------------------------// 

void Decoder::convertCurrentFrame(FrameBuf<RGB8>& rDestFrame) const
{
	if(!m_pVPX->pCurrentFrame)
	{
		return; // no frame available
	}

	const vpx_image_t& srcImage = *m_pVPX->pCurrentFrame;

	if(srcImage.fmt != VPX_IMG_FMT_I420)
		throw Decoder::Error("Unsupported image format. Only 4:2:0 allowed.");

	unsigned int c_w = srcImage.x_chroma_shift ? (1 + srcImage.d_w) >> srcImage.x_chroma_shift 
											   : srcImage.d_w;
	unsigned int c_h = srcImage.y_chroma_shift ? (1 + srcImage.d_h) >> srcImage.y_chroma_shift 
											   : srcImage.d_h;

	if(c_w * 2 != srcImage.d_w || c_h * 2 != srcImage.d_h)
		throw Decoder::Error("Unsupported chroma subsampling format.");

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