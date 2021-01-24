#pragma once
#include <libavformat/avformat.h>

int decoder_create(void * dec);
bool decoder_init(void *decoder, void *vb);
bool decoder_open(void * dec, const AVCodec *codec);
void decoder_close(void *dec);
bool decoder_push(void *dec, const AVPacket *packet);
void decoder_interrupt(void *dec);