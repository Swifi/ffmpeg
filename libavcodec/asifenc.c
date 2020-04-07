#include "libavutil/attributes.h"
#include "libavutil/float_dsp.h"
#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "mathops.h"

typedef struct ASIFFrameData ASIFFrameData;

struct ASIFFrameData
{

  AVFrame *refFrame;
  ASIFFrameData *nextFrame;

};

typedef struct ASIFContext 
{

  int debug_frame_count;
  int packet_created;
  int total_samples;
  int received_all_frames;
  ASIFFrameData *frame_data;

} ASIFContext;

static av_cold int asif_encode_init(AVCodecContext *avctx)
{
  ASIFContext * c = avctx->priv_data;
  c->received_all_frames = 0;
  c->frame_data = NULL;
  c->total_samples = 0;
  c->debug_frame_count = 0;
  c->packet_created = 0;

  // Initializing the context
  avctx->frame_size = 10000;
  avctx->bits_per_coded_sample = 8; 
  avctx->block_align           = avctx->channels * avctx->bits_per_coded_sample / 8;
  avctx->bit_rate              = 40000;

  return 0;
}

static int asif_send_frame(AVCodecContext *avctx, const AVFrame *frame)
{

  ASIFContext * c = avctx->priv_data;

  ASIFFrameData * currentFrame;

  // If last frame 
  if (frame == NULL) {
    av_log(NULL, AV_LOG_INFO, "Last frame received \n");
    c->received_all_frames = 1;

    avctx->sample_fmt = avctx->codec->sample_fmts[0];

    return 0;
  }

  c->total_samples += frame->nb_samples * avctx->channels;
  c->debug_frame_count++;

  // Make use of av_frame_ref() function or use memcpy
  // Allocate memory for arriving frames
  if (c->frame_data == NULL) {

    // Root frame
    c->frame_data = (ASIFFrameData*)malloc(sizeof(ASIFFrameData));
    currentFrame = c->frame_data;

    currentFrame->refFrame = av_frame_alloc();
    currentFrame->nextFrame = NULL;

  } else {

    // Find next postion to add frame
    currentFrame = c->frame_data;
    
    while (currentFrame->nextFrame != NULL) {
      currentFrame = currentFrame->nextFrame;
    }
    
    currentFrame->nextFrame = (ASIFFrameData*)malloc(sizeof(ASIFFrameData));

    currentFrame = currentFrame->nextFrame;
    currentFrame->refFrame = av_frame_alloc();
    currentFrame->nextFrame = NULL;

  }

  av_frame_ref(currentFrame->refFrame, frame);
  
  return 0;
}

static int asif_receive_packet(AVCodecContext *avctx, AVPacket *avpkt)
{
  ASIFContext *c = avctx->priv_data;
  int ret, n, channel;
  unsigned char *dst;
  ASIFFrameData *currentFrame;
  const uint8_t *samples_uint8_t;

  av_log(NULL, AV_LOG_INFO, "Operating in encoder receive packet \n");

  // Dont start writing a packet until all frames have been recieved
  if (c->received_all_frames == 0) {
     return AVERROR(EAGAIN);
  }

  if (c->packet_created == 1) {
    return AVERROR_EOF;
  }
  
  if ((ret = ff_alloc_packet2(avctx, avpkt, c->total_samples, c->total_samples)) < 0)
    return ret;
  dst = avpkt->data;

  // Loop through each channel
  for (channel = 0; channel < avctx->channels; channel++) {
    
    currentFrame = c->frame_data;
   
    // Loop through each node
    while (currentFrame != NULL) {
      int i;
      n = currentFrame->refFrame->nb_samples;
      samples_uint8_t = (const uint8_t *) currentFrame->refFrame->extended_data[channel];

      // Loop through each sample
      for (i = n; i > 0; i--) {
	register uint8_t v = *samples_uint8_t++;
      	bytestream_put_byte(&dst, v);
      }

      currentFrame = currentFrame->nextFrame;
    }
  }

  avctx->sample_fmt = avctx->codec->sample_fmts[0];

  c->packet_created = 1;

  return 0;
}

static av_cold int asif_encode_close(AVCodecContext *avctx)
{
  // TODO: Free all manually allocated memory
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
    .close          = asif_encode_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_SMALL_LAST_FRAME,
    .sample_fmts    = (const enum AVSampleFormat[]) { AV_SAMPLE_FMT_U8P, AV_SAMPLE_FMT_NONE},
};


