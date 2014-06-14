//-----------------------------------------------------------------------------------------------// 
// NesteggUtils.h
//-----------------------------------------------------------------------------------------------// 
#ifndef MPX_ANALYZE_NESTEGG_UTILS_H
#define MPX_ANALYZE_NESTEGG_UTILS_H

#include "nestegg/include/nestegg/nestegg.h"
#include <stdio.h>

namespace mpx {

//-----------------------------------------------------------------------------------------------// 

enum file_kind 
{
	RAW_FILE,
	IVF_FILE,
	WEBM_FILE
};

//-----------------------------------------------------------------------------------------------// 

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

//-----------------------------------------------------------------------------------------------// 

int file_is_webm(struct input_ctx* input,
						unsigned int* fourcc,
						unsigned int* width,
						unsigned int* height,
						unsigned int* fps_den,
						unsigned int* fps_num);

//-----------------------------------------------------------------------------------------------//

int read_frame(
	input_ctx *input,
	uint8_t** buf,
	size_t* buf_sz,
	size_t* buf_alloc_sz);

//-----------------------------------------------------------------------------------------------// 

} // mpx

//-----------------------------------------------------------------------------------------------// 

#endif
