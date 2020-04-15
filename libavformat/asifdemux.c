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

static int asif_read_packet(AVFormatContext *s, AVPacket *pkt)
{
  ASIFAudioDemuxerContext *s1 = s->priv_data;
  AVStream *st = s->streams[0];
  int ret;
  int size;

  av_log(NULL, AV_LOG_INFO, "Operating in Demuxer - Read Packet \n");

  if (st->codecpar->block_align <= 0)
    return AVERROR(EINVAL);
  
  // Populates the packet? Empty? or Filled with our packet?
  size = FFMAX(st->codecpar->sample_rate/25, 1);

  // ret = av_get_packet(s->pb, pkt, size);
  // av_append_packet and seek if writing 100 bytes per then seek forward samples per channel - 100.
  
  for (int i = 0; i < s1->channels; i++){
    ret = av_append_packet(s->pb, pkt, 100);
   
    if (ret < 0) {
      av_packet_unref(pkt);
      return ret;
    }

    avio_seek(s->pb, s1->total_samples/s1->channels - 100, SEEK_CUR);
  }
  
  pkt->stream_index = 0;

  return ret;
}

static int asif_probe(const AVProbeData *p)
{
  if (p->buf[0] == 'a' && p->buf[1] == 's' && p->buf[2] == 'i' && p->buf[3] == 'f')
    return AVPROBE_SCORE_MAX;
  else
    return 0;
}

static int asif_read_header(AVFormatContext *s)
{
  AVIOContext *pb = s->pb;
  AVCodecParameters *par;
  ASIFAudioDemuxerContext *s1 = s->priv_data;
  AVStream *st;
  uint32_t tag;
  uint32_t sample_rate;

  av_log(NULL, AV_LOG_INFO, "Operating in Demuxer - Header \n");
  

  st = avformat_new_stream(s, NULL);
  if (!st)
    return AVERROR(ENOMEM);

  par = s->streams[0]->codecpar;

  tag = avio_rl32(pb);
  if (tag != MKTAG('a','s','i','f'))
    return AVERROR_INVALIDDATA;

  st->codecpar->codec_id = AV_CODEC_ID_ASIF;
  st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;

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

  st->codecpar->block_align = st->codecpar->channels; 

  return 0;
}

AVInputFormat ff_asif_demuxer = {
  .name           = "asif",
  .long_name      = NULL_IF_CONFIG_SMALL("ASIF"),
  .priv_data_size = sizeof(ASIFAudioDemuxerContext),
  .read_probe     = asif_probe,
  .read_header    = asif_read_header,
  .read_packet    = asif_read_packet,
  .extensions     = "asif",
  .raw_codec_id   = AV_CODEC_ID_ASIF,
};
















