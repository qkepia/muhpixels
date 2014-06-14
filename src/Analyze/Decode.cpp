//-----------------------------------------------------------------------------------------------// 
// Decode.cpp
//-----------------------------------------------------------------------------------------------// 

#include <Decode.h>
#include <Utils.h>
#include <BitStream.h>
#include <NesteggUtils.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

#define VP9_FOURCC_MASK (0x00395056)

#pragma warning (disable: 4996) // shut up safety warning

namespace mpx {

//-----------------------------------------------------------------------------------------------// 

void modelBitStream(std::string file, 
					BitStreamInfo& info,
					FrameBuf<RGB8>& firstFrame)
{
	// try to open file
	const char* pFileName = file.c_str();
	if(!pFileName)
		throw DecodeError("Illegal Filename");

	FILE* pInFile = fopen(pFileName, "rb");
	if(!pInFile)
		throw DecodeError("Can't open file");
	
	// fill input context structure
	input_ctx input;
	memset(&input, 0, sizeof(input_ctx));
		
	// read first file info stuff
	input.infile = pInFile;
	unsigned int fourcc, width, height, fps_den, fps_num;
	if(file_is_webm(&input, &fourcc, &width, &height, &fps_den, &fps_num))
	{
		input.kind = WEBM_FILE;
	}
	else
	{
		throw DecodeError("Unrecognized input file type");
	}

	// check fourcc
	vpx_codec_iface_t* pCodecInterface = nullptr;
	if((fourcc & 0x00FFFFFF) !=  VP9_FOURCC_MASK)
		throw DecodeError("Only VP9 supported");
	
	pCodecInterface = vpx_codec_vp9_dx();

	vpx_codec_ctx_t decoder;
	vpx_codec_dec_cfg_t cfg = { 0 };
	int dec_flags = 0;	
	if(vpx_codec_dec_init(&decoder, pCodecInterface, &cfg, dec_flags)) 
	{
		throw DecodeError(sprint("Failed to initialize decoder: %s", vpx_codec_error(&decoder)));
	}

	int frame_avail = 1;
	int got_data = 0;

	// decode file
	int frame_in = 0;
	int frame_out = 0;
	int frames_corrupted = 0;
	while(frame_avail || got_data) 
	{
		vpx_codec_iter_t iter = NULL;
		vpx_image_t* img;
		int corrupted;

		frame_avail = 0;
		int stop_after = 0;
		
		if(!stop_after || frame_in < stop_after) 
		{
			uint8_t* buf = NULL;
			size_t buf_sz = 0, buf_alloc_sz = 0;
			if(!read_frame(&input, &buf, &buf_sz, &buf_alloc_sz)) 
			{
				frame_avail = 1;
				frame_in++;

				if(vpx_codec_decode(&decoder, buf, (unsigned int)buf_sz, NULL, 0)) 
				{
					std::string errorString = sprint("Failed to decode frame: %s", vpx_codec_error(&decoder));
					const char* pDetail = vpx_codec_error_detail(&decoder);
					if(pDetail)
						errorString += std::string(pDetail);

					throw DecodeError(errorString);
				}
			}
		}

		got_data = 0;
		if((img = vpx_codec_get_frame(&decoder, &iter))) 
		{
			++frame_out;
			got_data = 1;
		}

		if(vpx_codec_control(&decoder, VP8D_GET_FRAME_CORRUPTED, &corrupted)) 
		{
			throw DecodeError(sprint("Failed VP8_GET_FRAME_CORRUPTED: %s", vpx_codec_error(&decoder)));
		}

		frames_corrupted += corrupted;

		if(firstFrame.size() == 0)
		{
			if(img->fmt != VPX_IMG_FMT_I420)
				throw DecodeError("Unsupported image format. Only 4:2:0 allowed.");

			unsigned int c_w = img->x_chroma_shift ? (1 + img->d_w) >> img->x_chroma_shift : img->d_w;
			unsigned int c_h = img->y_chroma_shift ? (1 + img->d_h) >> img->y_chroma_shift : img->d_h;

			if(c_w * 2 != img->d_w || c_h * 2 != img->d_h)
				throw DecodeError("Unsupported chroma subsampling format.");

			uint32_t frameWidth = img->d_w;
			uint32_t frameHeight = img->d_h;
			firstFrame.setSize(frameWidth, frameHeight);

			int strideY = img->stride[VPX_PLANE_Y];
			int strideU = img->stride[VPX_PLANE_U];
			int strideV = img->stride[VPX_PLANE_V];

			for(uint32_t j = 0; j < frameHeight/2; j++)
			{
				const uint8_t* pY0 = img->planes[VPX_PLANE_Y] + 2 * j * strideY;
				const uint8_t* pY1 = pY0 + strideY;
				const uint8_t* pU = img->planes[VPX_PLANE_U] + j * strideU;
				const uint8_t* pV = img->planes[VPX_PLANE_V] + j * strideV;

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
					firstFrame(2*i, 2*j) = rgb0;
					firstFrame(2*i+1, 2*j) = rgb1;
					firstFrame(2*i, 2*j+1) = rgb2;
					firstFrame(2*i+1, 2*j+1) = rgb3;
				}
			}
		}

		if(stop_after && frame_in >= stop_after)
			break;
	}

	if(frames_corrupted)
		fprintf(stderr, "WARNING: %d frames corrupted.\n", frames_corrupted);

	// todo: RAIIIIII
	if(vpx_codec_destroy(&decoder)) 
	{
		throw DecodeError(sprint("Failed to destroy decoder: %s", vpx_codec_error(&decoder)));
	}

	if(input.nestegg_ctx)
		nestegg_destroy(input.nestegg_ctx);

	fclose(pInFile);
}

//-----------------------------------------------------------------------------------------------// 

} // mpx