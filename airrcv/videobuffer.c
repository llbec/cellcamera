#include <stdbool.h>
#include <stdlib.h>

// forward declarations
typedef struct AVFrame AVFrame;

struct video_buffer {
    AVFrame *decoding_frame;
    AVFrame *rendering_frame;
    //SDL_mutex *mutex;
    //bool render_expired_frames;
    //bool interrupted;
    //SDL_cond *rendering_frame_consumed_cond;
    bool rendering_frame_consumed;
    //struct fps_counter *fps_counter;
};

void * video_buffer_get_decoding_frame(void *vb)
{
    struct video_buffer * pvb = (struct video_buffer*) vb;
    if (!pvb || !pvb->decoding_frame) {
        return NULL;
    }
    return pvb->decoding_frame;
}

int video_buffer_create(void *vb)
{
    if(vb) {
        return -1;
    }
    vb = malloc(sizeof(struct video_buffer));
    if (!vb) {
        return -2;
    }
    return 0;
}

bool video_buffer_init(void *vb/*, struct fps_counter *fps_counter, bool render_expired_frames*/)
{
    if (!vb) {
        return false;
    }
    struct video_buffer * pvb = (struct video_buffer*) vb;

    //pvb->fps_counter = fps_counter;

    if (!(pvb->decoding_frame = av_frame_alloc())) {
        goto error_0;
    }

    if (!(pvb->rendering_frame = av_frame_alloc())) {
        goto error_1;
    }

    /*if (!(pvb->mutex = SDL_CreateMutex())) {
        goto error_2;
    }*/

    /*pvb->render_expired_frames = render_expired_frames;
    if (render_expired_frames) {
        if (!(pvb->rendering_frame_consumed_cond = SDL_CreateCond())) {
            SDL_DestroyMutex(pvb->mutex);
            goto error_2;
        }
        // interrupted is not used if expired frames are not rendered
        // since offering a frame will never block
        pvb->interrupted = false;
    }*/

    // there is initially no rendering frame, so consider it has already been
    // consumed
    pvb->rendering_frame_consumed = true;

    return true;

error_2:
    av_frame_free(&pvb->rendering_frame);
error_1:
    av_frame_free(&pvb->decoding_frame);
error_0:
    return false;
}

void video_buffer_destroy(void *vb)
{
    if (!vb) {
        return;
    }
    struct video_buffer * pvb = (struct video_buffer*) vb;
    /*if (pvb->render_expired_frames) {
        SDL_DestroyCond(pvb->rendering_frame_consumed_cond);
    }
    SDL_DestroyMutex(pvb->mutex);*/
    av_frame_free(&pvb->rendering_frame);
    av_frame_free(&pvb->decoding_frame);
}

static void video_buffer_swap_frames(void *vb)
{
    if (!vb) {
        return;
    }
    struct video_buffer * pvb = (struct video_buffer*) vb;
    AVFrame *tmp = pvb->decoding_frame;
    pvb->decoding_frame = pvb->rendering_frame;
    pvb->rendering_frame = tmp;
}

void video_buffer_offer_decoded_frame(void *vb, bool *previous_frame_skipped)
{
    if (!vb || !previous_frame_skipped) {
        return;
    }
    struct video_buffer * pvb = (struct video_buffer*) vb;
    /*mutex_lock(pvb->mutex);
    if (pvb->render_expired_frames) {
        // wait for the current (expired) frame to be consumed
        while (!pvb->rendering_frame_consumed && !pvb->interrupted) {
            cond_wait(pvb->rendering_frame_consumed_cond, pvb->mutex);
        }
    } else if (!pvb->rendering_frame_consumed) {
        fps_counter_add_skipped_frame(pvb->fps_counter);
    }*/

    video_buffer_swap_frames(pvb);

    *previous_frame_skipped = !pvb->rendering_frame_consumed;
    pvb->rendering_frame_consumed = false;

    //mutex_unlock(pvb->mutex);
}

const void * video_buffer_consume_rendered_frame(void *vb)
{
    if (!vb) {
        return;
    }
    struct video_buffer * pvb = (struct video_buffer*) vb;
    //assert(!vb->rendering_frame_consumed);
    if (pvb->rendering_frame_consumed) {
        return;
    }
    pvb->rendering_frame_consumed = true;
    /*fps_counter_add_rendered_frame(vb->fps_counter);
    if (vb->render_expired_frames) {
        // unblock video_buffer_offer_decoded_frame()
        cond_signal(vb->rendering_frame_consumed_cond);
    }*/
    return pvb->rendering_frame;
}

void video_buffer_interrupt(void *vb)
{
    /*if (vb->render_expired_frames) {
        mutex_lock(vb->mutex);
        vb->interrupted = true;
        mutex_unlock(vb->mutex);
        // wake up blocking wait
        cond_signal(vb->rendering_frame_consumed_cond);
    }*/
}