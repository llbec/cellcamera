#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mediaserver.h"
#include "buffer_util.h"

// airplay mirror resolution support:
// 2560 * 1440 
// 1920 * 1080
// 1280 * 720
#define SCREEN_WIDTH  2560
#define SCREEN_HEIGHT 1440

#define AIR_DEVICE_NAME  "Cam Receiver"
#define HEADER_SIZE 12
#define NO_PTS UINT64_C(-1)

const int MIRROR_BUFF_SIZE = 4 * 1024 * 1024;

typedef struct airplay_rcv_t {
	int audio_volume;
	uint8_t* mirrbuff;
	uint64_t ptsOrigin;
}  AirPlay_Rcv_t;

 AirPlay_Rcv_t  airplay_rcv;

#if 1
void cb_airplay_open(void *cls, char *url, float fPosition, double dPosition)
{}

void cb_airplay_play(void *cls)
{}

void cb_airplay_pause(void *cls)
{}

void cb_airplay_stop(void *cls)
{}

void cb_airplay_seek(void *cls, long fPosition)
{}
void cb_airplay_setvolume(void *cls, int volume)
{}
void cb_airplay_showphoto(void *cls, unsigned char *data, long long size)
{}

long cb_airplay_getduration(void *cls)
{
	//	Log(TEXT("AirPlayServer~~~~~~~:cb_airplay_pause"));
	return 0;
}

long cb_airplay_getpostion(void *cls)
{
	//Log(TEXT("AirPlayServer~~~~~~~:cb_airplay_seek"));
	return 0;
}
int cb_airplay_isplaying(void *cls)
{
	//	Log(TEXT("AirPlayServer~~~~~~~:cb_airplay_setvolume"));
	return 0;
}
int cb_airplay_ispaused(void *cls)
{
	return 0;
}

void sdl_audio_callback(void *cls, uint8_t *stream, int len)
{
}

//开始播放音频
void cb_audio_init(void *cls, int bits, int channels, int samplerate, int isaudio)
{}

//音频播放中
void cb_audio_process(void *cls, const void *buffer, int buflen, uint64_t timestamp, uint32_t seqnum)
{
}

//停止播放音频
void cb_audio_destory(void *cls)
{}

void cb_audio_setvolume(void *cls, int volume) //max 0  min  -30
{
	//printf("set volume %d \n", volume);
	int v = (int)((volume + 30) * 4.25 );
	(( AirPlay_Rcv_t *)cls)->audio_volume = v;
}

void cb_audio_setmetadata(void *cls, const void *buffer, int buflen)
{
}

void cb_audio_setcoverart(void *cls, const void *buffer, int buflen)
{
}

void cb_audio_flush(void *cls)
{
}

//开始播放
void mirroring_play(void *cls, int width, int height, const void *buffer, int buflen, int payloadtype, uint64_t timestamp)
{
	AirPlay_Rcv_t * src = ( AirPlay_Rcv_t *)cls;
	src->ptsOrigin = timestamp;
	//src->height = height;
	//src->width = width;
	if (!src->mirrbuff) {
		src->mirrbuff = malloc(MIRROR_BUFF_SIZE);
		if (!src->mirrbuff)
			return;
		memset(src->mirrbuff, 0, MIRROR_BUFF_SIZE);
	}

	int spscnt;
	int spsnalsize;
	int ppscnt;
	int ppsnalsize;

	unsigned    char *head = (unsigned  char *)buffer;

	spscnt = head[5] & 0x1f;
	spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
	ppscnt = head[8 + spsnalsize];
	ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);

	unsigned char *data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);

	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 1;

	memcpy(data + 4, head + 8, spsnalsize);

	data[4 + spsnalsize] = 0;
	data[5 + spsnalsize] = 0;
	data[6 + spsnalsize] = 0;
	data[7 + spsnalsize] = 1;

	memcpy(data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);

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

	src->ptsOrigin = timestamp;

	uint64_t pts = NO_PTS;
	uint32_t len = 4 + spsnalsize + 4 + ppsnalsize;
	uint8_t hdr[12];
	buffer_write64be(&hdr[0], pts);
	buffer_write32be(&hdr[8], len);
	//net_send_all(connfd, hdr, 12);
	//net_send_all(connfd, data, len);

	free(data);
}

//播放中
void mirroring_process(void *cls, const void *buffer, int buflen, int payloadtype, uint64_t timestamp)
{
	AirPlay_Rcv_t * src = ( AirPlay_Rcv_t *)cls;
	if (payloadtype == 0)
	{
		int		    rLen;
		unsigned    char *head;

		//unsigned char *data = (unsigned char *)malloc(buflen);
		//memcpy(data, buffer, buflen);
		memcpy(src->mirrbuff, buffer, buflen);

		rLen = 0;
		head = (unsigned char *)src->mirrbuff + rLen;
		while (rLen < buflen) {
			rLen += 4;
			rLen += (((uint32_t)head[0] << 24) | ((uint32_t)head[1] << 16) | ((uint32_t)head[2] << 8) | (uint32_t)head[3]);
			head[0] = 0;
			head[1] = 0;
			head[2] = 0;
			head[3] = 1;
			head = (unsigned char *)src->mirrbuff + rLen;
		}

		uint64_t pts = timestamp - src->ptsOrigin;;
		uint32_t len = buflen;
		uint8_t hdr[12];
		buffer_write64be(&hdr[0], pts);
		buffer_write32be(&hdr[8], len);
		//net_send_all(connfd, hdr, 12);
		//net_send_all(connfd, mirrbuff, len);
	}
	else if (payloadtype == 1)
	{
		int spscnt;
		int spsnalsize;
		int ppscnt;
		int ppsnalsize;

		unsigned    char *head = (unsigned  char *)buffer;

		spscnt = head[5] & 0x1f;
		spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
		ppscnt = head[8 + spsnalsize];
		ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);

		unsigned char *data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);

		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 1;

		memcpy(data + 4, head + 8, spsnalsize);
		data[4 + spsnalsize] = 0;
		data[5 + spsnalsize] = 0;
		data[6 + spsnalsize] = 0;
		data[7 + spsnalsize] = 1;

		memcpy(data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);

		src->ptsOrigin = timestamp;
		uint64_t pts = NO_PTS;
		uint32_t len = buflen;
		uint8_t hdr[12];
		buffer_write64be(&hdr[0], pts);
		buffer_write32be(&hdr[8], len);
		//net_send_all(connfd, hdr, 12);
		//net_send_all(connfd, data, len);

		free(data);
	}
}

//停止播放
void mirroring_stop(void *cls)
{
	 AirPlay_Rcv_t * src = ( AirPlay_Rcv_t *)cls;
	if (src->mirrbuff) {
		free(src->mirrbuff);
		src->mirrbuff = NULL;
	}
}

void mirroring_live(void *cls) {}
#endif

int airplay_recv_start()
{
	airplay_rcv.mirrbuff = NULL;
    airplay_callbacks_t ao = {
	    .cls = & airplay_rcv,
	    .AirPlayMirroring_Play = mirroring_play,
	    .AirPlayMirroring_Process = mirroring_process,
	    .AirPlayMirroring_Stop = mirroring_stop,
	    .AirPlayMirroring_Live = mirroring_live,

	    .AirPlayPlayback_Open = cb_airplay_open,
	    .AirPlayPlayback_Play = cb_airplay_play,
	    .AirPlayPlayback_Pause = cb_airplay_pause,
	    .AirPlayPlayback_Stop = cb_airplay_stop,
	    .AirPlayPlayback_Seek = cb_airplay_seek,
	    .AirPlayPlayback_SetVolume = cb_airplay_setvolume,
	    .AirPlayPlayback_ShowPhoto = cb_airplay_showphoto,
	    .AirPlayPlayback_GetDuration = cb_airplay_getduration,
	    .AirPlayPlayback_GetPostion = cb_airplay_getpostion,
	    .AirPlayPlayback_IsPlaying = cb_airplay_isplaying,
	    .AirPlayPlayback_IsPaused = cb_airplay_ispaused,
	    //...
	    .AirPlayAudio_Init = cb_audio_init,
	    .AirPlayAudio_Process = cb_audio_process,
	    .AirPlayAudio_destroy = cb_audio_destory,
	    .AirPlayAudio_SetVolume = cb_audio_setvolume,
	    .AirPlayAudio_SetMetadata = cb_audio_setmetadata,
	    .AirPlayAudio_SetCoverart = cb_audio_setcoverart,
	    .AirPlayAudio_Flush = cb_audio_flush,
    };
    return startMediaServer((char*)AIR_DEVICE_NAME, SCREEN_WIDTH, SCREEN_HEIGHT, &ao);
}

void airplay_recv_stop()
{
    stopMediaServer();
}
