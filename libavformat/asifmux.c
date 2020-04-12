#include <stdint.h>

#include "libavutil/intfloat.h"
#include "libavutil/opt.h"
#include "avformat.h"
#include "internal.h"
#include "asif.h"
#include "avio_internal.h"

typedef struct ASIFOutputContext { 
    const AVClass *class;
    int audio_stream_idx;
} ASIFOutputContext;

static int asif_write_header(AVFormatContext *s)
{
  ASIFOutputContext *asif = s->priv_data;
    AVIOContext *pb = s->pb;
    AVCodecParameters *par;
    uint32_t sample_rate;
    int i;

    asif->audio_stream_idx = -1;
    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        if (asif->audio_stream_idx < 0 && st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            asif->audio_stream_idx = i;
        } else if (st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            av_log(s, AV_LOG_ERROR, "ASIF allows only one audio stream and a picture.\n");
            return AVERROR(EINVAL);
        }
    }

    if (asif->audio_stream_idx < 0) {
        av_log(s, AV_LOG_ERROR, "No audio stream present.\n");
        return AVERROR(EINVAL);
    }

    par = s->streams[asif->audio_stream_idx]->codecpar;
    par->bits_per_coded_sample = 8; 

    /* ASIF header */
    ffio_wfourcc(pb, "asif");

    /* Common chunk */
    sample_rate = (uint32_t) par->sample_rate;

    avio_wl32(pb, sample_rate); /* Sample rate */

    avio_wl16(pb, par->channels);  /* Number of channels */

    // Number of samples per channel is written on the encoder itself

    return 0;
}

static int asif_write_packet(AVFormatContext *s, AVPacket *pkt)
{
    ASIFOutputContext *asif = s->priv_data;
    AVIOContext *pb = s->pb;


    av_log(NULL, AV_LOG_INFO, "Operating in muxer write packet \n");

    if (pkt->stream_index == asif->audio_stream_idx)
        avio_write(pb, pkt->data, pkt->size);

    return 0;
}

static const AVClass asif_muxer_class = {
  .class_name     = "ASIF_muxer",
};

AVOutputFormat ff_asif_muxer = {
    .name           = "asif",
    .long_name      = NULL_IF_CONFIG_SMALL("ASIF audio file (CS 3505 Spring 20202)"),
    .extensions     = "asif",
    .priv_data_size = sizeof(ASIFOutputContext),
    .audio_codec    = AV_CODEC_ID_ASIF,
    .write_header   = asif_write_header,
    .write_packet   = asif_write_packet,
    .priv_class     = &asif_muxer_class,
};






















