//Wow this is cool
#include "avcodec.h"

static av_cold int asif_encode_init(AVCodecContext *avctx)
{
  avctx->frame_size = 0; 
  avctx->bits_per_coded_sample = av_get_bits_per_sample(avctx->codec->id);
  avctx->block_align           = avctx->channels * avctx->bits_per_coded_sample / 8;
  avctx->bit_rate              = avctx->block_align * 8LL * avctx->sample_rate;

  return 0;
}

#define ENCODE(type, endian, src, dst, n, shift, offset)                \
    samples_ ## type = (const type *) src;                              \
    for (; n > 0; n--) {                                                \
        register type v = (*samples_ ## type++ >> shift) + offset;      \
        bytestream_put_ ## endian(&dst, v);                             \
    }

static int asif_encode_frame(AVCodecContext *avctx, AVPacket *avpkt,
			     const AVFrame *frame, int *got_packet_ptr)
{
  /*
   * Number of samples: n
   * Source pointer: src
   * Destination pointer: dst
   */
  
  bytestream_put_byte('a');
  bytestream_put_byte('s');
  bytestream_put_byte('i');
  bytestream_put_byte('f');
  bytestream_put_le32(/*sample_rate*/);
  bytestream_put_le16(/*number_of_channels*/);
  bytestream_put_le32(/*number_of_samples_per_channel*/);
  bytestream_put_uint8_t(/*Channel0_data*/);
  bytestream_put_uint8_t(/*Channel1_data*/);

  return 0;
}

AVCodec ff_asif_encoder = {
    .name           = "asif",
    .long_name      = NULL_IF_CONFIG_SMALL("ASIF audio file (CS 3505 Spring 20202)"),
    .id             = AV_CODEC_ID_ASIF,
    .type           = AVMEDIA_TYPE_AUDIO,
    //.priv_data_size
    .init           = asif_encode_init,
    .encode2        = asif_encode_frame,
    .sample_fmts    = NULL,
    //.capabilities
};
