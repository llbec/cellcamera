#include <stdbool.h>
#include <libavformat/avformat.h>
#include "decoder.h"

struct stream {
    //socket_t socket;
    void *video_buffer;
    //SDL_Thread *thread;
    void *decoder;
    //struct recorder *recorder;
    AVCodecContext *codec_ctx;
    AVCodecParserContext *parser;
    // successive packets may need to be concatenated, until a non-config
    // packet is available
    bool has_pending;
    AVPacket pending;
};

#define BUFSIZE 0x10000

#define HEADER_SIZE 12
#define NO_PTS UINT64_C(-1)

// The video stream contains raw packets, without time information. When we
// record, we retrieve the timestamps separately, from a "meta" header
// added by the server before each raw packet.
//
// The "meta" header length is 12 bytes:
// [. . . . . . . .|. . . .]. . . . . . . . . . . . . . . ...
//  <-------------> <-----> <-----------------------------...
//        PTS        packet        raw packet
//                    size
//
// It is followed by <packet_size> bytes containing the packet/frame.
static bool stream_recv_packet(struct stream *stream, AVPacket *packet,
                                uint8_t * phead, size_t headlen,
                                uint8_t * pdata, size_t datalen)
{
    if(!stream || !packet || phead || pdata) {
        return false;
    }
    uint8_t header[HEADER_SIZE];
    /*ssize_t r = net_recv_all(stream->socket, header, HEADER_SIZE);
    if (r < HEADER_SIZE) {
        return false;
    }*/
    if (headlen < HEADER_SIZE) {
        return false;
    }
    memcpy(header, phead, HEADER_SIZE);

    uint64_t pts = buffer_read64be(header);
    uint32_t len = buffer_read32be(&header[8]);
    assert(pts == NO_PTS || (pts & 0x8000000000000000) == 0);
    assert(len);
    if (av_new_packet(packet, len)) {
        //LOGE("Could not allocate packet");
        return false;
    }

    //r = net_recv_all(stream->socket, packet->data, len);
    if (datalen < 0 || ((uint32_t) datalen) < len) {
        av_packet_unref(packet);
        return false;
    }
    memcpy(packet->data, pdata, len);

    packet->pts = pts != NO_PTS ? (int64_t) pts : AV_NOPTS_VALUE;

    return true;
}

static void notify_stopped(void)
{
    //SDL_Event stop_event;
    //stop_event.type = EVENT_STREAM_STOPPED;
    //SDL_PushEvent(&stop_event);
}

static bool process_config_packet(struct stream *stream, AVPacket *packet)
{
    /*if (stream->recorder && !recorder_push(stream->recorder, packet)) {
        LOGE("Could not send config packet to recorder");
        return false;
    }*/
    return true;
}

static bool process_frame(struct stream *stream, AVPacket *packet)
{
    if (stream->decoder && !decoder_push(stream->decoder, packet)) {
        return false;
    }

    /*if (stream->recorder) {
        packet->dts = packet->pts;

        if (!recorder_push(stream->recorder, packet)) {
            LOGE("Could not send packet to recorder");
            return false;
        }
    }*/

    return true;
}

static bool stream_parse(struct stream *stream, AVPacket *packet)
{
    uint8_t *in_data = packet->data;
    int in_len = packet->size;
    uint8_t *out_data = NULL;
    int out_len = 0;
    int r = av_parser_parse2(stream->parser, stream->codec_ctx,
                             &out_data, &out_len, in_data, in_len,
                             AV_NOPTS_VALUE, AV_NOPTS_VALUE, -1);

    // PARSER_FLAG_COMPLETE_FRAMES is set
    assert(r == in_len);
    (void) r;
    assert(out_len == in_len);

    if (stream->parser->key_frame == 1) {
        packet->flags |= AV_PKT_FLAG_KEY;
    }

    bool ok = process_frame(stream, packet);
    if (!ok) {
        //LOGE("Could not process frame");
        return false;
    }

    return true;
}

static bool stream_push_packet(struct stream *stream, AVPacket *packet) {
    bool is_config = packet->pts == AV_NOPTS_VALUE;

    // A config packet must not be decoded immetiately (it contains no
    // frame); instead, it must be concatenated with the future data packet.
    if (stream->has_pending || is_config) {
        size_t offset;
        if (stream->has_pending) {
            offset = stream->pending.size;
            if (av_grow_packet(&stream->pending, packet->size)) {
                //LOGE("Could not grow packet");
                return false;
            }
        } else {
            offset = 0;
            if (av_new_packet(&stream->pending, packet->size)) {
                //LOGE("Could not create packet");
                return false;
            }
            stream->has_pending = true;
        }

        memcpy(stream->pending.data + offset, packet->data, packet->size);

        if (!is_config) {
            // prepare the concat packet to send to the decoder
            stream->pending.pts = packet->pts;
            stream->pending.dts = packet->dts;
            stream->pending.flags = packet->flags;
            packet = &stream->pending;
        }
    }

    if (is_config) {
        // config packet
        bool ok = process_config_packet(stream, packet);
        if (!ok) {
            return false;
        }
    } else {
        // data packet
        bool ok = stream_parse(stream, packet);

        if (stream->has_pending) {
            // the pending packet must be discarded (consumed or error)
            stream->has_pending = false;
            av_packet_unref(&stream->pending);
        }

        if (!ok) {
            return false;
        }
    }
    return true;
}
