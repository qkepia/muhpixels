//-----------------------------------------------------------------------------------------------// 
// NesteggUtils.cpp
//-----------------------------------------------------------------------------------------------// 

#include <NesteggUtils.h>
#include <stdlib.h>
#include <stdarg.h>

#define VP8_FOURCC_MASK (0x00385056)
#define VP9_FOURCC_MASK (0x00395056)

namespace mpx {

//-----------------------------------------------------------------------------------------------// 

unsigned int mem_get_le16(const void *vmem) 
{
	unsigned int  val;
	const unsigned char *mem = (const unsigned char *)vmem;

	val = mem[1] << 8;
	val |= mem[0];
	return val;
}

//-----------------------------------------------------------------------------------------------// 

unsigned int mem_get_le32(const void *vmem) 
{
	unsigned int  val;
	const unsigned char *mem = (const unsigned char *)vmem;

	val = mem[3] << 24;
	val |= mem[2] << 16;
	val |= mem[1] << 8;
	val |= mem[0];
	return val;
}

//-----------------------------------------------------------------------------------------------// 

#define IVF_FRAME_HDR_SZ (sizeof(uint32_t) + sizeof(uint64_t))
#define RAW_FRAME_HDR_SZ (sizeof(uint32_t))

//-----------------------------------------------------------------------------------------------// 

int read_frame(
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

//-----------------------------------------------------------------------------------------------// 

int nestegg_read_cb(void *buffer, size_t length, void *userdata)
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

//-----------------------------------------------------------------------------------------------// 

int nestegg_seek_cb(int64_t offset, int whence, void *userdata) 
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

//-----------------------------------------------------------------------------------------------// 

int64_t nestegg_tell_cb(void *userdata) 
{
	return ftell(static_cast<FILE*>(userdata));
}

//-----------------------------------------------------------------------------------------------// 

void nestegg_log_cb(nestegg *context, unsigned int severity, char const *format, ...) 
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

//-----------------------------------------------------------------------------------------------// 

int file_is_webm(struct input_ctx* input,
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

} // mpx