
/**
 * 最简单的基于FFmpeg的内存读写例子（内存播放器）
 * This software play video data in memory (not a file).
 * It's the simplest example to use FFmpeg to read from memory.
 *
 */

#include <windows.h>
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#include "libavutil/adler32.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

typedef struct
{
    int i32Size;
    int i32Off;
    void *pBegin;
    void *pBuff;
} VedioRecord_S;

//Callback
int read_buffer(void *opaque, uint8_t *buf, int buf_size)
{
	VedioRecord_S *ps = opaque;
    int i32Out, i32Ret;
    
    // printf("read size: %d\n", buf_size);
    // printf("%p, %p, %p, %d, %d\n", ps->pBegin, ps->pBuff, buf, ps->i32Off, ps->i32Size);
    
    i32Out = min(ps->i32Size - ps->i32Off, buf_size);
    i32Ret = (!i32Out ? AVERROR_EOF : i32Out);
    memcpy(buf, ps->pBegin + ps->i32Off, i32Out);
    ps->i32Off += i32Out;
    
    // printf("read out%d %d\n", i32Out, i32Ret);
    return (!i32Out ? AVERROR_EOF : i32Out);
}

int64_t seek_buffer(void *opaque, int64_t offset, int whence)
{
    VedioRecord_S *ps = opaque;
    int i32Tmp;
    
    switch (whence)
    {
    case SEEK_CUR:
        // printf("cur ");
        i32Tmp = ps->i32Off + offset;
        if (i32Tmp < 0 || i32Tmp > ps->i32Size)
        {
            return -1;
        }
        ps->i32Off = i32Tmp;
        break;
    case SEEK_END:
        // printf("end ");
        i32Tmp = ps->i32Size + offset;
        if (i32Tmp < 0 || i32Tmp > ps->i32Size)
        {
            return -1;
        }
        ps->i32Off = i32Tmp;
        // if (-1 == offset)
        // {
            // ps->i32Off = 0;
        // }
        break;
    case SEEK_SET:
        // printf("set ");
        if (offset < 0 || offset > ps->i32Size)
        {
            return -1;
        }
        ps->i32Off = offset;
        break;
    default:
        return -1;
    }
    
    // printf("off %d, out %d\n", offset, ps->i32Off);
    return ps->i32Off;
}

int main(int argc, char* argv[])
{
    AVCodec *codec = NULL;
    AVCodecContext *ctx= NULL;
    AVCodecParameters *origin_par = NULL;
    AVFrame *fr = NULL;
    uint8_t *byte_buffer = NULL;
    AVPacket pkt;
    AVFormatContext *fmt_ctx = NULL;
    int number_of_written_bytes;
    int video_stream;
    int got_frame = 0;
    int byte_buffer_size;
    int i = 0;
    int result;
    int end_of_stream = 0;
    int  ret = 0, i32Size;

    av_register_all();
    
#if 1
	// AVInputFormat   *piFmt = NULL;
    VedioRecord_S sRecord;
    FILE *pf;
    unsigned char *avBuff = (unsigned char *)av_malloc(655350);
    unsigned char *inBuff = (unsigned char *)av_malloc(655350);
    
    pf=fopen("1.h264","rb+");
    i32Size = fread(inBuff, 1, 655350, pf);
    sRecord.i32Size = i32Size;
    sRecord.pBegin = inBuff;
    sRecord.i32Off = 0;
    sRecord.pBuff = avBuff;
	avformat_network_init();
	
	//为在内存读取数据
	fmt_ctx = avformat_alloc_context();
	
	AVIOContext *avio = avio_alloc_context(avBuff, 655350, 0, &sRecord, read_buffer, NULL, seek_buffer);
	fmt_ctx->pb=avio;
    
	// ret = av_probe_input_buffer(avio, &piFmt, NULL, NULL, 0, 655350);
    // if (ret != 0)
    // {
        // printf("av_probe_input_buffer error %d", ret);
    // }
	
	// ret = avformat_open_input(&fmt_ctx,filepath,NULL,NULL);
	ret = avformat_open_input(&fmt_ctx,NULL,NULL,NULL);
	if(ret!=0)
	{
		printf("Couldn't open input stream %d.\n", ret);
		return -1;
	}    
    
#endif

#if 0
    result = avformat_open_input(&fmt_ctx, "1.h264", NULL, NULL);
    if (result < 0) {
        av_log(NULL, AV_LOG_ERROR, "Can't open file\n");
        return result;
    }
#endif
    
    result = avformat_find_stream_info(fmt_ctx, NULL);
    if (result < 0) {
        av_log(NULL, AV_LOG_ERROR, "Can't get stream info\n");
        return result;
    }

    video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream < 0) {
      av_log(NULL, AV_LOG_ERROR, "Can't find video stream in input file\n");
      return -1;
    }

    origin_par = fmt_ctx->streams[video_stream]->codecpar;

    codec = avcodec_find_decoder(origin_par->codec_id);
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "Can't find decoder\n");
        return -1;
    }

    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate decoder context\n");
        return AVERROR(ENOMEM);
    }

    result = avcodec_parameters_to_context(ctx, origin_par);
    if (result) {
        av_log(NULL, AV_LOG_ERROR, "Can't copy decoder context\n");
        return result;
    }

    result = avcodec_open2(ctx, codec, NULL);
    if (result < 0) {
        av_log(ctx, AV_LOG_ERROR, "Can't open decoder\n");
        return result;
    }

    fr = av_frame_alloc();
    if (!fr) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate frame\n");
        return AVERROR(ENOMEM);
    }

    byte_buffer_size = av_image_get_buffer_size(ctx->pix_fmt, ctx->width, ctx->height, 16);
    byte_buffer = av_malloc(byte_buffer_size);
    if (!byte_buffer) {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate buffer\n");
        return AVERROR(ENOMEM);
    }

    printf("#tb %d: %d/%d\n", video_stream, fmt_ctx->streams[video_stream]->time_base.num, fmt_ctx->streams[video_stream]->time_base.den);
    i = 0;
    av_init_packet(&pkt);
    do {
        if (!end_of_stream)
            if (av_read_frame(fmt_ctx, &pkt) < 0)
                end_of_stream = 1;
        if (end_of_stream) {
            pkt.data = NULL;
            pkt.size = 0;
        }
        if (pkt.stream_index == video_stream || end_of_stream) {
            got_frame = 0;
            if (pkt.pts == AV_NOPTS_VALUE)
                pkt.pts = pkt.dts = i;
            result = avcodec_decode_video2(ctx, fr, &got_frame, &pkt);
            if (result < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding frame\n");
                return result;
            }
            if (got_frame) {
                number_of_written_bytes = av_image_copy_to_buffer(byte_buffer, byte_buffer_size,
                                        (const uint8_t* const *)fr->data, (const int*) fr->linesize,
                                        ctx->pix_fmt, ctx->width, ctx->height, 1);
                if (number_of_written_bytes < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Can't copy image to buffer\n");
                    return number_of_written_bytes;
                }
                printf("%d, %10"PRId64", %10"PRId64", %8"PRId64", %8d, 0x%08lx\n", video_stream,
                        fr->pts, fr->pkt_dts, fr->pkt_duration,
                        number_of_written_bytes, av_adler32_update(0, (const uint8_t*)byte_buffer, number_of_written_bytes));
            }
            av_packet_unref(&pkt);
            av_init_packet(&pkt);
        }
        i++;
    } while (!end_of_stream || got_frame);

    av_packet_unref(&pkt);
    av_frame_free(&fr);
    avcodec_close(ctx);
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&ctx);
    av_freep(&byte_buffer);
    return 0;
}

