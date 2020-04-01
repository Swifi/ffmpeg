/**
 * @file
 * common header for ASIF muxer and demuxer
 */

#ifndef AVFORMAT_ASIF_H
#define AVFORMAT_ASIF_H

#include "avformat.h"
#include "internal.h"

int ff_asif_read_packet(AVFormatContext *s, AVPacket *pkt);

int ff_asif_read_seek(AVFormatContext *s,
		      int stream_index, int64_t timestamp, int flags);

#endif /* AVFORMAT_ASIF_H */












