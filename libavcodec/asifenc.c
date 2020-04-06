#include "libavutil/attributes.h"
#include "libavutil/float_dsp.h"
#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "mathops.h"

typedef struct ASIFFrameData ASIFFrameData;

struct ASIFFrameData
{

  uint8_t **sound_data;
  ASIFFrameData *otherFrame;

};

typedef struct ASIFContext 
{

  int sample_rate;
  int channels;
  int recieved_all_frames;
  ASIFFrameData *frame_data;

} ASIFContext;

static av_cold int asif_encode_init(AVCodecContext *avctx)
{
  ASIFContext * c = avctx->priv_data;
  c->recieved_all_frames = 0;
  c->frame_data = NULL;

  // Initializing the context
  avctx->frame_size = 1000;
  avctx->bits_per_coded_sample = 8; 
  avctx->block_align           = avctx->channels * avctx->bits_per_coded_sample / 8;
  avctx->bit_rate              = 40000;

  return 0;
}

static int asif_send_frame(AVCodecContext *avctx, const AVFrame *frame)
{
  int n, channel, sample_size, v, ret;

  ASIFContext * c = avctx->priv_data;

  ASIFFrameData * currentFrame;

  // If last frame 
  if (frame == NULL) {
    av_log(NULL, AV_LOG_INFO, "Last frame received");
    c->recieved_all_frames = 1;

    avctx->sample_fmt = avctx->codec->sample_fmts[0];

    return 0;
  }

  // Allocate memory for arriving frames
  if (c->frame_data == NULL) {
    // Root frame
    c->frame_data = (ASIFFrameData*)malloc(sizeof(ASIFFrameData));
    c->frame_data->sound_data = malloc(sizeof(uint8_t*) * avctx->channels);
    currentFrame = c->frame_data;
    currentFrame->otherFrame = NULL;
  } else {
    // Find next postion to add frame
    currentFrame = c->frame_data;
    
    while (currentFrame->otherFrame != NULL) {
      currentFrame = currentFrame->otherFrame;
    }
    
    currentFrame->otherFrame = (ASIFFrameData*)malloc(sizeof(ASIFFrameData));
    currentFrame->otherFrame->sound_data = malloc(sizeof(uint8_t) * avctx->channels);
    currentFrame = currentFrame->otherFrame;
    currentFrame->otherFrame = NULL;
  }
  
  sample_size = avctx->bits_per_coded_sample / 8;
  n           = frame->nb_samples * avctx->channels;
  n          /= avctx->channels;
  
  // Accumulate samples for each channel
  for (channel = 0; channel < avctx->channels; channel++) {
    currentFrame->sound_data[channel] = (const uint8_t *) frame->extended_data[channel];
  }
  
  return 0;
}

static int asif_receive_packet(AVCodecContext *avctx, AVPacket *avpkt)
{
  ASIFContext *c = avctx->priv_data;
  int ret;
  unsigned char *dst;
  int channel;
  ASIFFrameData *currentFrame;
  PutByteContext pb;

  // Dont start writing a packet until all frames have been recieved
  if (c->recieved_all_frames == 0) {
     return AVERROR(EAGAIN);
  }

  dst = avpkt->data;

  if ((ret = ff_alloc_packet2(avctx, avpkt, 16, 16)) < 0)
    return ret;

  // Loop through 
  // 2 channels 1 and 2
  // loop through 1
  // data[1] put in
  // next node
  // data[1] put in
  // next node
  // change channel
  // repeat

  currentFrame = c->frame_data;

  // Loop through each channel
  for (channel = 0; channel < avctx->channels; channel++) {
   
    // Loop through each node
    while (currentFrame != NULL) {
      uint8_t * frame_pointer = currentFrame->sound_data[channel];

      // Loop through each sample
      for (int i = 0; i < avctx->frame_size; i++) {
  	bytestream2_put_byte(&pb, frame_pointer[i]);
      }

      currentFrame = currentFrame->otherFrame;
    }
  }

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


