#include <stdint.h>

#include "libavutil/intfloat.h"
#include "libavutil/opt.h"
#include "avformat.h"
#include "internal.h"
#include "asif.h"
#include "avio_internal.h"
#include "isom.h"
#include "id3v2.h"

typedef struct ASIFOutputContext { 
    const AVClass *class;
    int64_t form;
    int64_t frames;
    int64_t ssnd;
    int audio_stream_idx;
    AVPacketList *pict_list, *pict_list_end;
    int write_id3v2;
    int id3v2_version;
} ASIFOutputContext;

static int asif_write_header(AVFormatContext *s)
{
  ASIFOutputContext *asif = s->priv_data;
    AVIOContext *pb = s->pb;
    AVCodecParameters *par;
    uint64_t sample_rate;
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
    par->bits_per_coded_sample = 8; // Forced ASK TA

    /* ASIF header */
    ffio_wfourcc(pb, "ASIF");

    /* Common chunk */
    sample_rate = av_double2int(par->sample_rate); // ASK TA WHY SAMPLE_RATE 0

    avio_wl32(pb, par->sample_rate); /* Sample rate */

    avio_wl16(pb, par->channels);  /* Number of channels */

    avio_wl32(pb, s->streams[0]->duration / par->channels); /* Number of samples per channel */

    asif->frames = avio_tell(pb);


    /* Sound data chunk */
    avpriv_set_pts_info(s->streams[asif->audio_stream_idx], 64, 1,
                        s->streams[asif->audio_stream_idx]->codecpar->sample_rate);

    return 0;
}

static int asif_write_packet(AVFormatContext *s, AVPacket *pkt)
{
    ASIFOutputContext *asif = s->priv_data;
    AVIOContext *pb = s->pb;
    if (pkt->stream_index == asif->audio_stream_idx)
        avio_write(pb, pkt->data, pkt->size);
    else {
        if (s->streams[pkt->stream_index]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
            return 0;

        /* warn only once for each stream */
        if (s->streams[pkt->stream_index]->nb_frames == 1) {
            av_log(s, AV_LOG_WARNING, "Got more than one picture in stream %d,"
                   " ignoring.\n", pkt->stream_index);
        }
        if (s->streams[pkt->stream_index]->nb_frames >= 1)
            return 0;

        return ff_packet_list_put(&asif->pict_list, &asif->pict_list_end,
                                  pkt, FF_PACKETLIST_FLAG_REF_PACKET);
    }

    return 0;
}

static const AVClass asif_muxer_class = {
  .class_name     = "ASIF_muxer",
  .item_name      = av_default_item_name,
  .version        = LIBAVUTIL_VERSION_INT,
};

AVOutputFormat ff_asif_muxer = {
    .name           = "asif",
    .long_name      = NULL_IF_CONFIG_SMALL("ASIF audio file (CS 3505 Spring 20202)"),
    .mime_type      = "audio/asif",
    .extensions     = "asif",
    .priv_data_size = sizeof(ASIFOutputContext),
    .audio_codec    = AV_CODEC_ID_ASIF,
    .video_codec    = AV_CODEC_ID_NONE,
    .write_header   = asif_write_header,
    .write_packet   = asif_write_packet,
    .codec_tag      = (const AVCodecTag* const []){ ff_codec_asif_tags, 0},
    .priv_class     = &asif_muxer_class,
    .flags          = AVFMT_NOTIMESTAMPS,
};






















