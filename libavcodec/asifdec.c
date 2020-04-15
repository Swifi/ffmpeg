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
    int c, n, ret, samples_per_block;
    volatile int sample_size;
    uint8_t *samples;
    int32_t *dst_int32_t;
    int8_t delta;
    
    av_log(NULL, AV_LOG_INFO, "Operating in Decoder - Frame Decoding \n");

    sample_size = avctx->bits_per_coded_sample / 8;

    samples_per_block = 1;

    if (sample_size == 0) {
        av_log(avctx, AV_LOG_ERROR, "Invalid sample_size\n");
        return AVERROR(EINVAL);
    }

    if (avctx->channels == 0) {
        av_log(avctx, AV_LOG_ERROR, "Invalid number of channels\n");
        return AVERROR(EINVAL);
    }

    if (avctx->codec_id != avctx->codec->id) {
        av_log(avctx, AV_LOG_ERROR, "codec ids mismatch\n");
        return AVERROR(EINVAL);
    }

    n = avctx->channels * sample_size;

    if (n && buf_size % n) {
        if (buf_size < n) {
            av_log(avctx, AV_LOG_ERROR,
                   "Invalid ASIF packet, data has size %d but at least a size of %d was expected\n",
                   buf_size, n);
            return AVERROR_INVALIDDATA;
        } else
            buf_size -= buf_size % n;
    }

    // n = buf_size / sample_size;
    n = 100;

    /* get output buffer */
    frame->nb_samples = n * avctx->channels;
    if ((ret = ff_get_buffer(avctx, frame, 0)) < 0) // Fills the frame
        return ret;
    
    //extended_data and loop over all channels

    for(int c = 0; c < avctx->channels; c++) {
      samples = frame->extended_data[c];

      for (; n > 0; n--)
      {
	*samples++ = *src++;
      }
    }
    
    // Adding data from packet (src) to frame (samples)
    av_log(avctx, AV_LOG_INFO, "Got out of for loop\n");

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
