#include "libavutil/attributes.h"
#include "libavutil/float_dsp.h"
#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "mathops.h"

/*
 * The solution for our encoder by Vijay Bajracharya
 * and Nick Hayes. 4/16/2020
 */

typedef struct ASIFFrameData ASIFFrameData;

/*
 * The struct to hold each frame data while
 * we accumulate all of the frames (linked list)
 */
struct ASIFFrameData
{
  AVFrame *refFrame;
  ASIFFrameData *nextFrame;

};

/*
 * The struct to hold all of our main information and the
 * first frame information.
 */
typedef struct ASIFContext 
{
  int packet_created;
  int total_samples;
  int received_all_frames;
  ASIFFrameData *frame_data;

} ASIFContext;

/*
 * Our initializer function, setting all of our main information and getting
 * our encoder ready for getting all of the frames.
 */
static av_cold int asif_encode_init(AVCodecContext *avctx)
{
  // Initializing our ASIFContext
  ASIFContext * c = avctx->priv_data;
  c->received_all_frames = 0;
  c->frame_data = NULL;
  c->total_samples = 0;
  c->packet_created = 0;

  // Initializing the context
  avctx->frame_size = 10000;
  avctx->bits_per_coded_sample = 8; 
  avctx->block_align           = avctx->channels * avctx->bits_per_coded_sample / 8;
  avctx->bit_rate              = 40000;

  return 0;
}
/* 
 * Our main function for accumulating all of the frame data
 * to our linked list.
 */
static int asif_send_frame(AVCodecContext *avctx, const AVFrame *frame)
{

  ASIFContext * c = avctx->priv_data;

  ASIFFrameData * currentFrame;

  // If last frame 
  if (frame == NULL) {
    c->received_all_frames = 1;

    avctx->sample_fmt = avctx->codec->sample_fmts[0];

    return 0;
  }

  c->total_samples += frame->nb_samples * avctx->channels;

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
    // Add the frame and initialize the data
    currentFrame->nextFrame = (ASIFFrameData*)malloc(sizeof(ASIFFrameData));

    currentFrame = currentFrame->nextFrame;
    currentFrame->refFrame = av_frame_alloc();
    currentFrame->nextFrame = NULL;

  }

  av_frame_ref(currentFrame->refFrame, frame);
  
  return 0;
}
/*
 * Our function which receives the main packet and writes to the file.
 * Will only run if we have received all frames.
 * This will iterate through all of the frames.
 */
static int asif_receive_packet(AVCodecContext *avctx, AVPacket *avpkt)
{
  // Initialize data variables
  ASIFContext *c = avctx->priv_data;
  int ret, n, channel, first_sample_encoded;
  unsigned char *dst;
  ASIFFrameData *currentFrame;
  const uint8_t *samples_uint8_t;
  register int32_t old_value;
  register int32_t new_value;
  register int32_t delta;

  // Dont start writing a packet until all frames have been recieved
  if (c->received_all_frames == 0) {
     return AVERROR(EAGAIN);
  }

  if (c->packet_created == 1) {
    return AVERROR_EOF;
  }

  if ((ret = ff_alloc_packet2(avctx, avpkt, c->total_samples + 4, c->total_samples + 4)) < 0)
    return ret;
  dst = avpkt->data;

  // Put the total number of samples for channel in the bytestream
  bytestream_put_le32(&dst, c->total_samples / avctx->channels);

  // Loop through each channel
  for (channel = 0; channel < avctx->channels; channel++) {
    
    currentFrame = c->frame_data;

    // Set to 0 to get the first value to be equal to the actual value of the program
    first_sample_encoded = 1;
    old_value = *currentFrame->refFrame->extended_data[channel];

    // Cap the old value to unsigned int of 8 bits
    if(old_value > 255) {
      //find what our sample is currently at
      //set the old value to be max
      old_value = 255;
    }

    bytestream_put_byte(&dst, (uint8_t) old_value);

   
    // Loop through each node
    while (currentFrame != NULL) {
      int i;
      n = currentFrame->refFrame->nb_samples;
      samples_uint8_t = (const uint8_t *) currentFrame->refFrame->extended_data[channel] + first_sample_encoded;

      // Loop through each sample
      for (i = n - first_sample_encoded; i > 0; i--) {
	new_value = *samples_uint8_t++;
	delta = new_value - old_value;

	// See if old_value is above or below the 8 bit max
	if(delta > 127) {
	  //find what our sample is currently at
	  new_value = old_value + 127;
	  //set the old value to be max
	  delta = 127;
	  }
	else if(delta < -128) {
	  //find what our sample is
	  new_value = old_value - 128;
	  //set the old value to just be at max
	  delta = -128;
	}

	// Write to byte_stream
	bytestream_put_byte(&dst, (int8_t) delta);

	old_value = new_value;
      }
      first_sample_encoded = 0;
      currentFrame = currentFrame->nextFrame;
    }
  }

  avctx->sample_fmt = avctx->codec->sample_fmts[0];
  c->packet_created = 1;

  return 0;
}
/*
 * The closing function which will delete all of our manually allocated
 * data and return 0 saying everything went okay.
 */
static av_cold int asif_encode_close(AVCodecContext *avctx)
{
  ASIFFrameData * temp;
  ASIFContext * c = avctx->priv_data;
  ASIFFrameData * current_frame  = c->frame_data;;

  while(current_frame != NULL)
    {
      // set us to the current_frame
      temp = current_frame;
      //get the next frame
      current_frame = current_frame->nextFrame;
      //free the frame we were at
      free(temp);
    }
  //everything should be freed, return
  
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


