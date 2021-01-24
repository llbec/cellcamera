#include <stdbool.h>
#include "videobuffer.h"
#include "decoder.h"

struct decoder {
    void *video_buffer;
    AVCodecContext *codec_ctx;
};

int decoder_create(void * dec)
{
    if (dec) {
        return -1;
    }
    dec = malloc(sizeof(struct decoder));
    if (!dec) {
        return -2;
    }
    return 0;
}

// set the decoded frame as ready for rendering, and notify
static void push_frame(struct decoder *decoder)
{
    bool previous_frame_skipped;
    video_buffer_offer_decoded_frame(decoder->video_buffer,
                                     &previous_frame_skipped);
    if (previous_frame_skipped) {
        // the previous EVENT_NEW_FRAME will consume this frame
		printf("the previous EVENT_NEW_FRAME will consume this frame\n");
        return;
    }
    /*static SDL_Event new_frame_event = {
        .type = EVENT_NEW_FRAME,
    };
    SDL_PushEvent(&new_frame_event);*/
}

bool decoder_init(void *decoder, void *vb)
{
    if(!decoder || !vb) {
        return false;
    }
    ((struct decoder *)decoder)->video_buffer = vb;
    return true;
}

bool decoder_open(void * dec, const AVCodec *codec)
{
    if(!dec || !codec) {
        return false;
    }
    struct decoder * pdecr = (struct decoder *) dec;
    pdecr->codec_ctx = avcodec_alloc_context3(codec);
    if (!pdecr->codec_ctx) {
        LOGC("Could not allocate decoder context");
        return false;
    }

    if (avcodec_open2(pdecr->codec_ctx, codec, NULL) < 0) {
        LOGE("Could not open codec");
        avcodec_free_context(&pdecr->codec_ctx);
        return false;
    }

    return true;
}

void decoder_close(void *dec)
{
    if (!dec) {
        return;
    }
    struct decoder * decoder = (struct decoder *)dec;
    avcodec_close(decoder->codec_ctx);
    avcodec_free_context(&decoder->codec_ctx);
}

bool decoder_push(void *dec, const AVPacket *packet)
{
    if(!dec || !packet) {
        return false;
    }
    struct decoder * decoder = (struct decoder *)dec;
    AVFrame * picture = (AVFrame *)video_buffer_get_decoding_frame(decoder->video_buffer);
// the new decoding/encoding API has been introduced by:
// <http://git.videolan.org/?p=ffmpeg.git;a=commitdiff;h=7fc329e2dd6226dfecaa4a1d7adf353bf2773726>
#ifdef SCRCPY_LAVF_HAS_NEW_ENCODING_DECODING_API
    int ret;
    if ((ret = avcodec_send_packet(decoder->codec_ctx, packet)) < 0) {
        LOGE("Could not send video packet: %d", ret);
        return false;
    }
    ret = avcodec_receive_frame(decoder->codec_ctx, picture);
    if (!ret) {
        // a frame was received
        push_frame(decoder);
    } else if (ret != AVERROR(EAGAIN)) {
        LOGE("Could not receive video frame: %d", ret);
        return false;
    }
#else
    int got_picture;
    int len = avcodec_decode_video2(decoder->codec_ctx, picture, &got_picture, packet);
    if (len < 0) {
        LOGE("Could not decode video packet: %d", len);
        return false;
    }
    if (got_picture) {
        push_frame(decoder);
    }
#endif
    return true;
}

void decoder_interrupt(void *dec)
{
    if (!dec) {
        return;
    }
    struct decoder * decoder = (struct decoder *)dec;
    video_buffer_interrupt(decoder->video_buffer);
}