#include "libavutil/attributes.h"
#include "libavutil/float_dsp.h"
#include "avcodec.h"
#include "bytestream.h"
#include "internal.h"
#include "mathops.h"

static av_cold int asif_encode_init(AVCodecContext *avctx)
{
    avctx->frame_size = 0;
    avctx->bits_per_coded_sample = 8; //av_get_bits_per_sample(avctx->codec->id);
    avctx->block_align           = avctx->channels * avctx->bits_per_coded_sample / 8;
    avctx->bit_rate              = 40000;  //avctx->block_align * 8LL * avctx->sample_rate;
    return 0;
}

static int asif_encode_frame(AVCodecContext *avctx, AVPacket *avpkt,
                            const AVFrame *frame, int *got_packet_ptr)
{
   int n, c, sample_size, v, ret;
    const short *samples;
    unsigned char *dst;
    const uint8_t *samples_uint8_t;

    sample_size = avctx->bits_per_coded_sample / 8;
    n           = frame->nb_samples * avctx->channels;
    samples     = (const short *)frame->data[0];

    if ((ret = ff_alloc_packet2(avctx, avpkt, n * sample_size, n * sample_size)) < 0)
        return ret;
    dst = avpkt->data;

    /* samples_uint8_t = (const uint8_t *) samples; */
    /* for (; n > 0; n--) { */
    /*     register uint8_t v = (*samples_uint8_t++ >> 0) - 128; */
    /* 	bytestream_put_byte(&dst, v); */
    /* } */

    n /= avctx->channels;

    for (c = 0; c < avctx->channels; c++) {
        int i;
        samples_uint8_t = (const uint8_t *) frame->extended_data[c];
        for (i = n; i > 0; i--) {
            register uint8_t v = (*samples_uint8_t++ >> 0) - 128;
    	    bytestream_put_byte(&dst, v);
        }
    }

    avctx->sample_fmt = avctx->codec->sample_fmts[0];

    *got_packet_ptr = 1;
    return 0;
}

AVCodec ff_asif_encoder = {
    .name           = "asif",
    .long_name      = NULL_IF_CONFIG_SMALL("ASIF audio file (CS 3505 Spring 20202)"),
    .id             = AV_CODEC_ID_ASIF,
    .type           = AVMEDIA_TYPE_AUDIO,
    .init           = asif_encode_init,
    .encode2        = asif_encode_frame,
    .capabilities   = AV_CODEC_CAP_VARIABLE_FRAME_SIZE,
    .sample_fmts    = (const enum AVSampleFormat[]) { AV_SAMPLE_FMT_U8P, AV_SAMPLE_FMT_NONE},
};


