#include "avformat.h"
#include "internal.h"
#include "asif.h"
#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "libavutil/avassert.h"
#include "libavutil/mathematics.h"

typedef struct ASIFAudioDemuxerContext {
  int sample_rate;
  int channels;
  int total_samples;
} ASIFAudioDemuxerContext;


int ff_asif_read_packet(AVFormatContext *s, AVPacket *pkt)
{
  ASIFAudioDemuxerContext *s1;
  AVCodecParameters *par = s->streams[0]->codecpar;
  int ret, size;

  if (par->block_align <= 0)
    return AVERROR(EINVAL);
  
  ret = av_get_packet(s->pb, pkt, s1->total_samples);
  pkt->stream_index = 0;

  return ret;
}

static int asif_read_header(AVFormatContext *s)
{
  AVIOContext *pb = s->pb;
  AVCodecParameters *par;
  ASIFAudioDemuxerContext *s1 = s->priv_data;
  AVStream *st;
  uint32_t tag;
  uint32_t sample_rate;
  

  st = avformat_new_stream(s, NULL);
  if (!st)
    return AVERROR(ENOMEM);

  par = s->streams[0]->codecpar;

  tag = avio_rl32(pb);
  if (tag != MKTAG('A','S','I','F'))
    return AVERROR_INVALIDDATA;

  // Extract sample rate from file
  sample_rate = avio_rl32(pb);
  par->sample_rate = sample_rate;
  s1->sample_rate = sample_rate;

  // Extract number of channels from file
  par->channels = avio_rl16(pb);
  s1->channels = par->channels;

  // Extract total samples from the file
  s1->total_samples = avio_rl32(pb) * par->channels;
 
  par->bits_per_coded_sample = 8;

  return 0;
}

AVInputFormat ff_asif_demuxer = {
  .name           = "asif",
  .long_name      = NULL_IF_CONFIG_SMALL("ASIF"),
  .priv_data_size = sizeof(ASIFAudioDemuxerContext),
  .read_header    = asif_read_header,
  .read_packet    = ff_asif_read_packet,
  .flags          = AVFMT_GENERIC_INDEX,
  .extensions     = "asif",
  .raw_codec_id   = AV_CODEC_ID_ASIF,
};
















