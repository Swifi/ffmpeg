#include "libavutil/attributes.h"
#include "libavutil/float_dsp.h"
#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "mathops.h"

static av_cold int asif_decode_init(AVCodecContext *avctx)
{
  av_log(NULL, AV_LOG_INFO, "Operating in Decoder - Initializer \n");

  if (avctx->channels <= 0) {
    av_log(avctx, AV_LOG_ERROR, "ASIF channels out of bounds\n");
    return AVERROR(EINVAL);
  }

  avctx->sample_fmt = avctx->codec->sample_fmts[0];

  return 0;
}

static int asif_decode_frame(AVCodecContext *avctx, void *data,
                            int *got_frame_ptr, AVPacket *avpkt)
{
    const uint8_t *src = avpkt->data;
    int buf_size       = avpkt->size;
    AVFrame *frame     = data;
    int n, ret;
    uint8_t *samples;
    uint8_t first_sample, current_sample;
    int8_t delta;
    
    av_log(NULL, AV_LOG_INFO, "Operating in Decoder - Frame Decoding \n");

    n = buf_size / avctx->channels;

    /* get output buffer */
    frame->nb_samples = n;

    if ((ret = ff_get_buffer(avctx, frame, 0)) < 0) // Fills the frame
        return ret;
    
    //extended_data and loop over all channels

    for(int c = 0; c < avctx->channels; c++) {
      n = buf_size / avctx->channels;
      samples = frame->extended_data[c];

      // Decode first sample
      first_sample = (uint8_t) *src;
      *samples = first_sample;

      src++;
      samples++;

      current_sample = first_sample;

      // Decode the rest of the samples
      for (; n - 1 > 0; n--) {
    	delta = (int8_t) *src;
    	current_sample = current_sample + delta;
        *samples = current_sample;
    
    	src++;
    	samples++;
      }
    }

    *got_frame_ptr = 1;

    return buf_size;
}


AVCodec ff_asif_decoder = {
    .name           = "asif",
    .long_name      = NULL_IF_CONFIG_SMALL("ASIF audio file (CS 3505 Spring 20202)"),
    .type           = AVMEDIA_TYPE_AUDIO,
    .id             = AV_CODEC_ID_ASIF,                                   
    .init           = asif_decode_init,                                                                  
    .decode         = asif_decode_frame,
    .capabilities   = AV_CODEC_CAP_DR1, 
    .sample_fmts    = (const enum AVSampleFormat[]) { AV_SAMPLE_FMT_U8P, AV_SAMPLE_FMT_NONE},
};
