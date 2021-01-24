#pragma once

int video_buffer_create(void *vb);
bool video_buffer_init(void *vb);
void video_buffer_destroy(void *vb);
void video_buffer_offer_decoded_frame(void *vb, bool *previous_frame_skipped);

// return a AVFrame ptr
const void * video_buffer_consume_rendered_frame(void *vb);
void video_buffer_interrupt(void *vb);

void * video_buffer_get_decoding_frame(void *vb);