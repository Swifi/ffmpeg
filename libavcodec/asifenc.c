#include "libavutil/attributes.h"
#include "libavutil/float_dsp.h"
#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "mathops.h"

typedef struct ASIFContext 
{

  int sample_rate;
  int channels;
  uint8_t sound_data;
  ASIFContext *other;

} ASIFContext;

static av_cold int asif_encode_init(AVCodecContext *avctx)
{
  // Initializing the context
  avctx->frame_size = 10000;
  avctx->bits_per_coded_sample = 8; 
  avctx->block_align           = avctx->channels * avctx->bits_per_coded_sample / 8;
  avctx->bit_rate              = 40000;

  ASIFContext c = avctx->priv_data;

  for(int i = 1; i < avctx->channels; i++){
    c->other = (struct ASIFContext*)malloc(sizeof(struct ASIFContext));
  }

  return 0;
}

int asif_send_frame(AVCodecContext *avctx, const AVFrame *frame)
{
  int n, c, sample_size, v, ret;
  const uint8_t *samples_uint8_t;
  
  sample_size = avctx->bits_per_coded_sample / 8;
  n           = frame->nb_samples * avctx->channels;
  n          /= avctx->channels;
  
  for (c = 0; c < avctx->channels; c++) {
    int i;
    samples_uint8_t = (const uint8_t *) frame->extended_data[c];
    for (i = n; i > 0; i--) {
      register uint8_t v = (*samples_uint8_t++ >> 0) - 128;
      // Accumulate specific channel data to struct member
    }
  }

  avctx->sample_fmt = avctx->codec->sample_fmts[0];
  
  return 0;
}

int asif_receive_packet(AVCodecContext *avctx, AVPacket *avpkt)
{
  int ret;
  
  if ((ret = ff_alloc_packet2(avctx, avpkt, 16, 16)) < 0)
    return ret;

  return 0;
}

AVCodec ff_asif_encoder = {
    .name           = "asif",
    .long_name      = NULL_IF_CONFIG_SMALL("ASIF audio file (CS 3505 Spring 20202)"),
    .priv_data_size = sizeof(ASIFContext),
    .id             = AV_CODEC_ID_ASIF,
    .type           = AVMEDIA_TYPE_AUDIO,
    .init           = asif_encode_init,
    .send_frame     = asif_send_frame,
    .receive_packet = asif_receive_packet,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_SMALL_LAST_FRAME,
    .sample_fmts    = (const enum AVSampleFormat[]) { AV_SAMPLE_FMT_U8P, AV_SAMPLE_FMT_NONE},
};


