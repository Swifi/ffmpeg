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
  register int8_t old_value;
  uint8_t first_sample;
  /* register int8_t catch_up = 0; */

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

    // Put in first byte for the channel
    // This will be a value from 0 - 255, we don't need to clamp
    first_sample = *currentFrame->refFrame->extended_data[channel];
    // Since this is a signed int, we need to get a value from -128 to 127
    old_value = first_sample - 128;

    bytestream_put_byte(&dst, first_sample);
   
    // Loop through each node
    while (currentFrame != NULL) {
      int i;
      n = currentFrame->refFrame->nb_samples;
      samples_uint8_t = (const uint8_t *) currentFrame->refFrame->extended_data[channel];

      // Loop through each sample
      for (i = n; i > 0; i--) {
	register int8_t new_value = *samples_uint8_t++;
	old_value = new_value - old_value;


	//TODO: Fix this, this is with Delta
	/* register int8_t previous_catch_up; */
	/* previous_catch_up = catch_up; */

	/* // Find if we can add v and catch_up for this next sample */

	/* // DEBUGGING */
	/* printf("%s", "\nNew value before difference: "); */
	/* printf("%d", b); */
	/* printf("%s", "\nOld Value before difference: "); */
	/* printf("%d", old_value); */

	/* // Find if we will need a catch_up for the next sample */
	/* catch_up = asif_check_overflow_subtraction(old_value, new_value); */

	/* // If adding catch_up and previous_catch up has no overflow, just add them, otherwise */
	/* // just set catch_up to be the max amount it can be */
	/* if(asif_check_overflow_addition(catch_up, previous_catch_up) == 0) { */
	/*   catch_up = catch_up + previous_catch_up; */
	/*   } */
	/* else { */
	/*   if(catch_up > 0) */
	/*     catch_up = 127; */
	/*   else */
	/*     catch_up = -128; */
	/* } */
	
	/* // Find the delta by taking the new value minus the old value (and the catch_up) */
	/* // TODO: check overflow */
	/* // TODO: set catch up accordingly after */
	/* old_value = new_value - (old_value - catch_up); */

	// DEBUGGING
	/* printf("%s", "\nOld Value after difference: "); */
	/* printf("%d", old_value); */

	bytestream_put_byte(&dst, old_value);

	// Change the value of old_value to new_value (instead of a delta, let it be the value)
	old_value = new_value;
      }
	

      currentFrame = currentFrame->nextFrame;
    }
  }


  avctx->sample_fmt = avctx->codec->sample_fmts[0];

  c->packet_created = 1;

  return 0;
}

/* // This will return a new catch_up value */
/* static int8_t asif_check_overflow_subtraction(int8_t & v, int8_t & b) */
/* { */
/*   int8_t catch_up = 0; */
/*   // Find the difference between b (what we just got) and v (the previous value) and assign it to v */
/*   // to be the delta */
/*   if((b > 0) && (v < (-128 + b))) { */
/*     // We went too far positive */
/*     printf("%s", "\nMade it to the catch up if statement"); */
/*   } */
/*   else if ((b < 0) && (v > (127 + b))) { */
/*     // We went too far negative */
/*     printf("%s", "\nMade it to the catch up if statement"); */
/*   } */
/*   // No overflow, set catch_up to 0 */
/*   else */
/*     catch_up = 0; */

/*   return catch_up; */
/* } */

/* static int8_t asif_check_overflow_addition(int8_t & v, int8_t & b) */
/* { */
/*   int8_t catch_up = 0; */
/*   // Find the difference between b (what we just got) and v (the previous value) and assign it to v */
/*   // to be the delta */
/*   if((b > 0) && (v > (127 - b))) { */
/*     // We went too far positive */
/*     printf("%s", "\nMade it to the catch up if statement"); */
/*   } */
/*   else if ((b < 0) && (v < (-128 - b))) { */
/*     // We went too far negative */
/*     printf("%s", "\nMade it to the catch up if statement"); */
/*   } */
/*   // No overflow, set catch_up to 0 */
/*   else */
/*     catch_up = 0; */

/*   return catch_up; */
/* } */

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


