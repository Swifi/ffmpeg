#include "libavutil/attributes.h"
#include "libavutil/float_dsp.h"
#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "mathops.h"

static av_cold int asif_decode_init(AVCodecContext *avctx)
{
  if (avctx->channels <= 0) {
    av_log(avctx, AV_LOG_ERROR, "ASIF channels out of bounds\n");
    return AVERROR(EINVAL);
  }

  return 0;
}

static int asif_decode_frame(AVCodecContext *avctx, void *data,
                            int *got_frame_ptr, AVPacket *avpkt)
{
    const uint8_t *src = avpkt->data;
    int buf_size       = avpkt->size;
    AVFrame *frame     = data;
    int sample_size, c, n, ret, samples_per_block;
    uint8_t *samples;
    int32_t *dst_int32_t;

    sample_size = avctx->bits_per_coded_sample / 8;

    /* av_get_bits_per_sample returns 0 for AV_CODEC_ID_ASIF_DVD */
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

    n = buf_size / sample_size;

    /* get output buffer */
    frame->nb_samples = n * samples_per_block / avctx->channels;
    if ((ret = ff_get_buffer(avctx, frame, 0)) < 0)
        return ret;
    samples = frame->data[0];
    
    for (; n > 0; n--)
      *samples++ = *src++ + 128;
    

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

};
