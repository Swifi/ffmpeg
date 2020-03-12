//Wow this is cool
#include "avcodec.h"

static av_cold int asif_encode_init(AVCodecContext *avctx)
{
  avctx->codec_id = AV_CODEC_ID_ASIF;
}

static int asif_encode_frame(AVCodecContext *avctx, AVPacket *avpkt,
			     const AVFrame *frame, int *got_packet_ptr)
{

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
