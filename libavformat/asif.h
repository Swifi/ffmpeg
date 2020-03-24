/**
 * @file
 * common header for ASIF muxer and demuxer
 */

#ifndef AVFORMAT_ASIF_H
#define AVFORMAT_ASIF_H

#include "avformat.h"
#include "internal.h"

static const AVCodecTag ff_codec_asif_tags[] = {
    { AV_CODEC_ID_ASIF,         MKTAG('A','S','I','F') },
    { AV_CODEC_ID_NONE,         0 },
};

#endif /* AVFORMAT_ASIF_H */












