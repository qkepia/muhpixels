//-----------------------------------------------------------------------------------------------// 
// Decode.cpp
//-----------------------------------------------------------------------------------------------// 

#include <Analyze/Decode.h>
#include <Base/Utils.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx_config.h"
#include "vpx/vpx_decoder.h"
#include "vpx_ports/vpx_timer.h"

//#if CONFIG_VP9_DECODER
#include "vpx/vp8dx.h"
//#endif


#define VP8_FOURCC (0x30385056)
#define VP9_FOURCC (0x30395056)
#define VP8_FOURCC_MASK (0x00385056)
#define VP9_FOURCC_MASK (0x00395056)
//tools_common.h(19): error C2371: 'off_t' : redefinition; different basic types
//#include "tools_common.h"




#include "nestegg/include/nestegg/nestegg.h"
//#include "third_party/libyuv/include/libyuv/scale.h"

struct iface_struct
{
  char const *name;
  const vpx_codec_iface_t *(*iface)(void);
  unsigned int             fourcc;
  unsigned int             fourcc_mask;
};

static const iface_struct ifaces[] =
{
	{"vp9",  vpx_codec_vp9_dx, VP9_FOURCC_MASK, 0x00FFFFFF},
};



#pragma warning (disable: 4996)

namespace mpx { namespace Analyze {

class DecodeError : std::exception 
{
public:
	DecodeError() {}
	DecodeError(std::string error) : m_error(error) {}
	const char* what() const { return m_error.c_str(); }
private:
	std::string m_error;
};

//vpx_codec_ctx_t
//vpx_codec_dec_cfg_t
//vpx_codec_iface_t
//vpx_image_t
//vpx_codec_control
//vpx_codec_dec_init
//vpx_codec_decode
//vpx_codec_destroy
//vpx_codec_error
//vpx_codec_error_detail
//vpx_codec_get_frame
//vpx_codec_iface_t
//vpx_codec_iter_t
//vpx_image_t
//vpx_img_alloc
//vpx_img_free
//vpx_usec_timer
//vpx_usec_timer_elapsed
//vpx_usec_timer_mark
//vpx_usec_timer_start

static unsigned int mem_get_le16(const void *vmem) 
{
	unsigned int  val;
	const unsigned char *mem = (const unsigned char *)vmem;

	val = mem[1] << 8;
	val |= mem[0];
	return val;
}

static unsigned int mem_get_le32(const void *vmem) 
{
	unsigned int  val;
	const unsigned char *mem = (const unsigned char *)vmem;

	val = mem[3] << 24;
	val |= mem[2] << 16;
	val |= mem[1] << 8;
	val |= mem[0];
	return val;
}

enum file_kind 
{
  RAW_FILE,
  IVF_FILE,
  WEBM_FILE
};

// qk: directly copied from vpxdec.c
struct input_ctx 
{
  enum file_kind  kind;
  FILE           *infile;
  nestegg        *nestegg_ctx;
  nestegg_packet *pkt;
  unsigned int    chunk;
  unsigned int    chunks;
  unsigned int    video_track;
};

#define IVF_FRAME_HDR_SZ (sizeof(uint32_t) + sizeof(uint64_t))
#define RAW_FRAME_HDR_SZ (sizeof(uint32_t))

static int read_frame(
	input_ctx *input,
	uint8_t** buf,
	size_t* buf_sz,
	size_t* buf_alloc_sz) 
{
	char raw_hdr[IVF_FRAME_HDR_SZ];
	size_t new_buf_sz;
	FILE* infile = input->infile;
	file_kind kind = input->kind;
	
	if(kind == WEBM_FILE) 
	{
		if(input->chunk >= input->chunks) 
		{
			unsigned int track;

			do 
			{
				/* End of this packet, get another. */
				if(input->pkt)
					nestegg_free_packet(input->pkt);

				if(nestegg_read_packet(input->nestegg_ctx, &input->pkt) <= 0
					|| nestegg_packet_track(input->pkt, &track))
					return 1;

			} while (track != input->video_track);

			if(nestegg_packet_count(input->pkt, &input->chunks))
				return 1;

			input->chunk = 0;
		}

		if(nestegg_packet_data(input->pkt, input->chunk, buf, buf_sz))
			return 1;

		input->chunk++;

		return 0;
	}
	else if(fread(raw_hdr, kind == IVF_FILE ? IVF_FRAME_HDR_SZ : RAW_FRAME_HDR_SZ, 1, infile) != 1) 
	{
		/* For both the raw and ivf formats, the frame size is the first 4 bytes
		 * of the frame header. We just need to special case on the header
		 * size.
		 */
		if(!feof(infile))
			fprintf(stderr, "Failed to read frame size\n");

		new_buf_sz = 0;
	}
	else 
	{
		new_buf_sz = mem_get_le32(raw_hdr);
		if(new_buf_sz > 256 * 1024 * 1024) 
		{
			fprintf(stderr, "Error: Read invalid frame size (%u)\n",
				(unsigned int)new_buf_sz);
			new_buf_sz = 0;
		}

		if(kind == RAW_FILE && new_buf_sz > 256 * 1024)
		{
			// todo: throw
			// fprintf(stderr, "Warning: Read invalid frame size (%u) - not a raw file?\n", (unsigned int)new_buf_sz);
		}

		if(new_buf_sz > *buf_alloc_sz) 
		{
			uint8_t *new_buf = static_cast<uint8_t*>(realloc(*buf, 2 * new_buf_sz));

			if(new_buf) {
				*buf = new_buf;
				*buf_alloc_sz = 2 * new_buf_sz;
			}
			else {
				fprintf(stderr, "Failed to allocate compressed data buffer\n");
				new_buf_sz = 0;
			}
		}
	}

	*buf_sz = new_buf_sz;

	if(!feof(infile)) 
	{
		if(fread(*buf, 1, *buf_sz, infile) != *buf_sz) 
		{
			fprintf(stderr, "Failed to read full frame\n");
			return 1;
		}
		return 0;
	}
	return 1;
}


static int nestegg_read_cb(void *buffer, size_t length, void *userdata)
{
	FILE *f = static_cast<FILE*>(userdata);

	if(fread(buffer, 1, length, f) < length) 
	{
		if(ferror(f))
			return -1;
		if(feof(f))
			return 0;
	}
	return 1;
}


static int nestegg_seek_cb(int64_t offset, int whence, void *userdata) 
{
	switch (whence) 
	{
	case NESTEGG_SEEK_SET:
		whence = SEEK_SET;
		break;
	case NESTEGG_SEEK_CUR:
		whence = SEEK_CUR;
		break;
	case NESTEGG_SEEK_END:
		whence = SEEK_END;
		break;
	};
	return fseek(static_cast<FILE*>(userdata), (long)offset, whence) ? -1 : 0;
}


static int64_t nestegg_tell_cb(void *userdata) 
{
	return ftell(static_cast<FILE*>(userdata));
}


static void nestegg_log_cb(nestegg *context, unsigned int severity, char const *format, ...) 
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

static int file_is_webm(struct input_ctx* input,
						unsigned int* fourcc,
						unsigned int* width,
						unsigned int* height,
						unsigned int* fps_den,
						unsigned int* fps_num) 
{
	unsigned int i, n;
	int          track_type = -1;
	int          codec_id;

	nestegg_io io = { nestegg_read_cb, nestegg_seek_cb, nestegg_tell_cb, 0 };
	nestegg_video_params params;

	io.userdata = input->infile;
	if(nestegg_init(&input->nestegg_ctx, io, NULL))
		goto fail;

	if(nestegg_track_count(input->nestegg_ctx, &n))
		goto fail;

	for (i = 0; i < n; i++) {
		track_type = nestegg_track_type(input->nestegg_ctx, i);

		if(track_type == NESTEGG_TRACK_VIDEO)
			break;
		else if(track_type < 0)
			goto fail;
	}

	codec_id = nestegg_track_codec_id(input->nestegg_ctx, i);
	if(codec_id == NESTEGG_CODEC_VP8) {
		*fourcc = VP8_FOURCC_MASK;
	}
	else if(codec_id == NESTEGG_CODEC_VP9) {
		*fourcc = VP9_FOURCC_MASK;
	}
	else {
		fprintf(stderr, "Not VPx video, quitting.\n");
		exit(1);
	}

	input->video_track = i;

	if(nestegg_track_video_params(input->nestegg_ctx, i, &params))
		goto fail;

	*fps_den = 0;
	*fps_num = 0;
	*width = params.width;
	*height = params.height;
	return 1;
fail:
	input->nestegg_ctx = NULL;
	rewind(input->infile);
	return 0;
}


//-----------------------------------------------------------------------------------------------// 

void modelBitStream(std::string file, Model::BitStreamInfo& info)
{
	//vpx_codec_ctx_t        decoder;
	//char                  *pFileName = NULL;
	//int                    i;
	//uint8_t               *buf = NULL;
	//size_t                 buf_sz = 0, buf_alloc_sz = 0;
	//FILE                  *pInFile;
	//int                    frame_in = 0, frame_out = 0, flipuv = 0, noblit = 0, do_md5 = 0, progress = 0;
	//int                    stop_after = 0, postproc = 0, summary = 0, quiet = 1;
	//int                    arg_skip = 0;
	//int                    ec_enabled = 0;
	//vpx_codec_iface_t       *iface = NULL;
	//unsigned int           fourcc;
	//unsigned long          dx_time = 0;
	//struct arg               arg;
	//char                   **argv, **argi, **argj;
	//const char             *outfile_pattern = 0;
	//char                    outfile[PATH_MAX];
	//int                     single_file;
	//int                     use_y4m = 1;
	//unsigned int            width;
	//unsigned int            height;
	//unsigned int            fps_den;
	//unsigned int            fps_num;
	//void                   *out = NULL;
	//vpx_codec_dec_cfg_t     cfg = { 0 };
	//struct input_ctx        input = { 0 };
	//int                     frames_corrupted = 0;
	//int                     dec_flags = 0;
	//int                     do_scale = 0;
	//int                     stream_w = 0, stream_h = 0;
	//vpx_image_t             *scaled_img = NULL;
	//int                     frame_avail, got_data;

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

	//if(use_y4m && !noblit) 
	//{
	//	char buffer[128];

	//	if(!single_file) {
	//		fprintf(stderr, "YUV4MPEG2 not supported with output patterns,"
	//			" try --i420 or --yv12.\n");
	//		return EXIT_FAILURE;
	//	}

	//	if(input.kind == WEBM_FILE)
	//	if(webm_guess_framerate(&input, &fps_den, &fps_num)) {
	//		fprintf(stderr, "Failed to guess framerate -- error parsing "
	//			"webm file?\n");
	//		return EXIT_FAILURE;
	//	}


	//	/*Note: We can't output an aspect ratio here because IVF doesn't
	//		store one, and neither does VP8.
	//		That will have to wait until these tools support WebM natively.*/
	//	snprintf(buffer, sizeof(buffer), "YUV4MPEG2 W%u H%u F%u:%u I%c ",
	//		width, height, fps_num, fps_den, 'p');
	//	out_put(out, (unsigned char *)buffer,
	//		(unsigned int)strlen(buffer), do_md5);
	//}

	/* Try to determine the codec from the fourcc. */
	vpx_codec_iface_t* pCodecInterface = nullptr;
	for(int i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++)
	{
		if((fourcc & ifaces[i].fourcc_mask) == ifaces[i].fourcc) 
		{
			vpx_codec_iface_t  *ivf_iface = ifaces[i].iface();

			if(pCodecInterface && pCodecInterface != ivf_iface)
				fprintf(stderr, "Notice -- IVF header indicates codec: %s\n", ifaces[i].name);
			else
				pCodecInterface = ivf_iface;

			break;
		}
	}

  //int frame_in = 0;
  //int frame_out = 0;
  //int flipuv = 0; 
  //int noblit = 0; 
  //int do_md5 = 0; 
  //int progress = 0;
  //int int stop_after = 0; 
  //int postproc = 0; 
  //int summary = 0;
  //int quiet = 1;
  //int ec_enabled = 0;

	vpx_codec_ctx_t decoder;
	vpx_codec_dec_cfg_t cfg = { 0 };
	int dec_flags = 0;	
	if(vpx_codec_dec_init(&decoder, pCodecInterface ? pCodecInterface : ifaces[0].iface(), &cfg, dec_flags)) 
	{
		throw DecodeError(Base::sprint("Failed to initialize decoder: %s", vpx_codec_error(&decoder)));
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
					std::string errorString = Base::sprint("Failed to decode frame: %s", vpx_codec_error(&decoder));
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
			throw DecodeError(Base::sprint("Failed VP8_GET_FRAME_CORRUPTED: %s", vpx_codec_error(&decoder)));
		}

		frames_corrupted += corrupted;

		//// todo: wut if(!noblit) 
		{
			//if(frame_out == 1 && img && use_y4m) 
			//{
			//	/* Write out the color format to terminate the header line */
			//	const char *color =
			//		img->fmt == VPX_IMG_FMT_444A ? "C444alpha\n" :
			//		img->fmt == VPX_IMG_FMT_I444 ? "C444\n" :
			//		img->fmt == VPX_IMG_FMT_I422 ? "C422\n" :
			//		"C420jpeg\n";

			//	out_put(out, (const unsigned char*)color, strlen(color), do_md5);
			//}
				
			//if(img) 
			//{
			//	unsigned int y;
			//	char out_fn[PATH_MAX];
			//	uint8_t *buf;
			//	unsigned int c_w =
			//		img->x_chroma_shift ? (1 + img->d_w) >> img->x_chroma_shift
			//		: img->d_w;
			//	unsigned int c_h =
			//		img->y_chroma_shift ? (1 + img->d_h) >> img->y_chroma_shift
			//		: img->d_h;

			//	if(!single_file) 
			//	{
			//		size_t len = sizeof(out_fn)-1;

			//		out_fn[len] = '\0';
			//		generate_filename(outfile_pattern, out_fn, len - 1,
			//			img->d_w, img->d_h, frame_in);
			//		out = out_open(out_fn, do_md5);
			//	}
			//	else if(use_y4m)
			//	{
			//		out_put(out, (unsigned char *)"FRAME\n", 6, do_md5);
			//	}

			//	buf = img->planes[VPX_PLANE_Y];

			//	for (y = 0; y < img->d_h; y++) 
			//	{
			//		out_put(out, buf, img->d_w, do_md5);
			//		buf += img->stride[VPX_PLANE_Y];
			//	}

			//	buf = img->planes[flipuv ? VPX_PLANE_V : VPX_PLANE_U];

			//	for (y = 0; y < c_h; y++) 
			//	{
			//		out_put(out, buf, c_w, do_md5);
			//		buf += img->stride[VPX_PLANE_U];
			//	}

			//	buf = img->planes[flipuv ? VPX_PLANE_U : VPX_PLANE_V];

			//	for (y = 0; y < c_h; y++) 
			//	{
			//		out_put(out, buf, c_w, do_md5);
			//		buf += img->stride[VPX_PLANE_V];
			//	}

			//	if(!single_file)
			//		out_close(out, out_fn, do_md5);
			//}
		}

		if(stop_after && frame_in >= stop_after)
			break;
	}

	if(frames_corrupted)
		fprintf(stderr, "WARNING: %d frames corrupted.\n", frames_corrupted);

//fail:

	// todo: RAIIIIII

	if(vpx_codec_destroy(&decoder)) 
	{
		throw DecodeError(Base::sprint("Failed to destroy decoder: %s", vpx_codec_error(&decoder)));
	}

	if(input.nestegg_ctx)
		nestegg_destroy(input.nestegg_ctx);

	fclose(pInFile);
}

//-----------------------------------------------------------------------------------------------// 

}} // mpx::Analyze