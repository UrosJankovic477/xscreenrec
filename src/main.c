#define _GNU_SOURCE
#include <xscreenrec.h>

inline struct timespec get_elapsed_time(struct timespec begin, struct timespec end)
{
    return (struct timespec){end.tv_sec - begin.tv_sec, end.tv_nsec - begin.tv_nsec};
}

int main(int argc, char const **argv)
{
    struct timespec begin, end;
    timespec_get(&begin, TIME_UTC);
    const char *filename = "../video.mp4";
    const char *encoder_name = "mpeg4";
    int status;
    FILE *file;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc(); 
    xcb_connection_t *connection = xcb_connect(NULL, NULL);

    const AVCodec *codec = avcodec_find_encoder_by_name(encoder_name);
    if (codec == NULL)
    {
        fprintf(stderr, "Can't find %s\n", encoder_name);
        exit(1);
    }
    
    
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(connection));
    xcb_screen_t *screen = screen_iterator.data;
    
    AVCodecContext *avctx;
    avctx = avcodec_alloc_context3(codec);
    avctx->bit_rate = 1000000;
    avctx->width = screen->width_in_pixels;
    avctx->height = screen->height_in_pixels;
    avctx->framerate = (AVRational){25, 1};
    avctx->time_base = (AVRational){1, 25};

    avctx->gop_size = 10;
    avctx->max_b_frames = 1;
    avctx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (codec->id == AV_CODEC_ID_H264)
    {
        av_opt_set(avctx->priv_data, "preset", "slow", 0);
    }
    
    xcb_window_t window = screen->root;
    uint32_t value = 1;
    xcb_gcontext_t gc = xcb_generate_id(connection);

    status = avcodec_open2(avctx, codec, NULL);
    if (status < 0)
    {
        exit(1);
    }
     

    file = fopen(filename, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "Can't open %s\n", filename);
        exit(1);
    }
    
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    frame->format = avctx->pix_fmt;
    frame->width  = avctx->width;
    frame->height = avctx->height;
    status = av_frame_get_buffer(frame, 32);
    if (status < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    
    xcb_create_gc(connection, gc, window, XCB_GC_SUBWINDOW_MODE, &value);

    timespec_get(&end, TIME_UTC);
    xsr_context srctx;
    srctx.connection = connection;
    srctx.elapsed_time = end.tv_sec - begin.tv_sec + ((double)(end.tv_nsec - begin.tv_nsec)) / 1000000000.0;
    srctx.pts = 0;
    srctx.screen = screen;

    XsrStartRecording(&srctx, avctx, frame, packet, file);

    avcodec_free_context(&avctx);
    av_frame_free(&frame);
    av_packet_free(&packet);

    avcodec_close(avctx);
    
    fclose(file);
    printf("writen to %s\n", filename);

    xcb_disconnect(connection);


    return 0;
}

