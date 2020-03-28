#include "avformat.h"
#include "internal.h"
#include "asif.h"
#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "libavutil/avassert.h"
#include "libavutil/mathematics.h"

typedef struct ASIFAudioDemuxerContext {
    AVClass *class;
    int sample_rate;
    int channels;
} ASIFAudioDemuxerContext;

#define RAW_SAMPLES     1024

int ff_asif_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    AVCodecParameters *par = s->streams[0]->codecpar;
    int ret, size;

    if (par->block_align <= 0)
        return AVERROR(EINVAL);

    /*
     * Compute read size to complete a read every 62ms.
     * Clamp to RAW_SAMPLES if larger.
     */
    size = FFMAX(par->sample_rate/25, 1);
    size = FFMIN(size, RAW_SAMPLES) * par->block_align;

    ret = av_get_packet(s->pb, pkt, size);

    pkt->flags &= ~AV_PKT_FLAG_CORRUPT;
    pkt->stream_index = 0;

    return ret;
}

int ff_asif_read_seek(AVFormatContext *s,
                     int stream_index, int64_t timestamp, int flags)
{
    AVStream *st;
    int block_align, byte_rate;
    int64_t pos, ret;

    st = s->streams[0];

    block_align = st->codecpar->block_align ? st->codecpar->block_align :
        (av_get_bits_per_sample(st->codecpar->codec_id) * st->codecpar->channels) >> 3;
    byte_rate = st->codecpar->bit_rate ? st->codecpar->bit_rate >> 3 :
        block_align * st->codecpar->sample_rate;

    if (block_align <= 0 || byte_rate <= 0)
        return -1;
    if (timestamp < 0) timestamp = 0;

    /* compute the position by aligning it to block_align */
    pos = av_rescale_rnd(timestamp * byte_rate,
                         st->time_base.num,
                         st->time_base.den * (int64_t)block_align,
                         (flags & AVSEEK_FLAG_BACKWARD) ? AV_ROUND_DOWN : AV_ROUND_UP);
    pos *= block_align;

    /* recompute exact position */
    st->cur_dts = av_rescale(pos, st->time_base.den, byte_rate * (int64_t)st->time_base.num);
    if ((ret = avio_seek(s->pb, pos + s->internal->data_offset, SEEK_SET)) < 0)
        return ret;
    return 0;
}

static int asif_read_header(AVFormatContext *s)
{
    ASIFAudioDemuxerContext *s1 = s->priv_data;
    AVStream *st;
    uint8_t *mime_type = NULL;

    st = avformat_new_stream(s, NULL);
    if (!st)
        return AVERROR(ENOMEM);


    st->codecpar->codec_type  = AVMEDIA_TYPE_AUDIO;
    st->codecpar->codec_id    = s->iformat->raw_codec_id;
    st->codecpar->sample_rate = s1->sample_rate;
    st->codecpar->channels    = s1->channels;

    av_opt_get(s->pb, "mime_type", AV_OPT_SEARCH_CHILDREN, &mime_type);
    if (mime_type && s->iformat->mime_type) {
        int rate = 0, channels = 0, little_endian = 0;
        size_t len = strlen(s->iformat->mime_type);
        if (!av_strncasecmp(s->iformat->mime_type, mime_type, len)) { /* audio/L16 */
            uint8_t *options = mime_type + len;
            len = strlen(mime_type);
            while (options < mime_type + len) {
                options = strstr(options, ";");
                if (!options++)
                    break;
                if (!rate)
                    sscanf(options, " rate=%d",     &rate);
                if (!channels)
                    sscanf(options, " channels=%d", &channels);
                if (!little_endian) {
                     char val[14]; /* sizeof("little-endian") == 14 */
                     if (sscanf(options, " endianness=%13s", val) == 1) {
                         little_endian = strcmp(val, "little-endian") == 0;
                     }
                }
            }
            if (rate <= 0) {
                av_log(s, AV_LOG_ERROR,
                       "Invalid sample_rate found in mime_type \"%s\"\n",
                       mime_type);
                av_freep(&mime_type);
                return AVERROR_INVALIDDATA;
            }
            st->codecpar->sample_rate = rate;
            if (channels > 0)
                st->codecpar->channels = channels;
            if (little_endian)
                st->codecpar->codec_id = AV_CODEC_ID_ASIF;
        }
    }
    av_freep(&mime_type);

    st->codecpar->bits_per_coded_sample = 8; // FIGURE OUT WHAT GOES IN HERE
      // av_get_bits_per_sample(st->codecpar->codec_id);

    av_assert0(st->codecpar->bits_per_coded_sample > 0);

    st->codecpar->block_align =
        st->codecpar->bits_per_coded_sample * st->codecpar->channels / 8;

    avpriv_set_pts_info(st, 64, 1, st->codecpar->sample_rate);
    return 0;
}
          
static const AVClass asif_demuxer_class = {            
    .class_name = "asif demuxer",                        
    .item_name  = av_default_item_name,                                                  
    .version    = LIBAVUTIL_VERSION_INT,                    
};      

AVInputFormat ff_asif_demuxer = {
    .name           = "asif",
    .long_name      = NULL_IF_CONFIG_SMALL("ASIF"),
    .priv_data_size = sizeof(ASIFAudioDemuxerContext),
    .read_header    = asif_read_header,
    .read_packet    = ff_asif_read_packet,
    .read_seek      = ff_asif_read_seek,
    .flags          = AVFMT_GENERIC_INDEX,
    .extensions     = "asif",
    .raw_codec_id   = AV_CODEC_ID_ASIF,
    .priv_class     = &asif_demuxer_class,
};

