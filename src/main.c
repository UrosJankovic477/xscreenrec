#define _GNU_SOURCE
#include <xsr/xscreenrec.h>
#include <unistd.h>

inline struct timespec get_elapsed_time(struct timespec begin, struct timespec end)
{
    return (struct timespec){end.tv_sec - begin.tv_sec, end.tv_nsec - begin.tv_nsec};
}

int main(int argc, char const **argv)
{
    struct timespec begin, end;
    timespec_get(&begin, TIME_UTC);

    XsrReadOptions(argc, argv);

    int status;
    FILE *file;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc(); 
    xcb_connection_t *connection = xcb_connect(NULL, NULL);

    const AVCodec *codec = avcodec_find_encoder(XsrGetOption(XSR_OPT_CODEC).codec.codecid);
    
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(connection));
    xcb_screen_t *screen = screen_iterator.data;
    
    AVCodecContext *avctx;
    avctx = avcodec_alloc_context3(codec);
    avctx->bit_rate = XsrGetOption(XSR_OPT_BITRATE).bit_rate.bit_rate;
    avctx->width = screen->width_in_pixels;
    avctx->height = screen->height_in_pixels;
    avctx->framerate = XsrGetOption(XSR_OPT_FRAMERATE).framerate.framerate;
    avctx->time_base = (AVRational){1, avctx->framerate.num};

    avctx->gop_size = 10;
    avctx->max_b_frames = 1;
    avctx->pix_fmt = XsrGetOption(XSR_OPT_PIXFMT).pixfmt.pixfmt;
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
     
    char filename[256];
    const char *outpath_opt = XsrGetOption(XSR_OPT_OUTPUTPATH).outputpath.path;
    if (outpath_opt == NULL)
    {
        char hms[] = __TIME__;
        const char *h = strtok(hms, ":");
        const char *m = strtok(NULL, ":");
        const char *s = strtok(NULL, ":");
        //const char *user = getlogin();
        //if (user == NULL)
        //{
        //    fprintf(stderr, "Can't open file try running with \'sudo\'\n");
        //}
        strcpy(filename, "../test.mp4");
        //sprintf(filename, "/home/%s/Videos/Screen recording %s %s-%s-%s.mp4", user, __DATE__, h, m, s);
    }
    else
    {
        strncpy(filename, outpath_opt, 256);
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
    status = av_frame_get_buffer(frame, 0);
    if (status < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    
    xcb_create_gc(connection, gc, window, XCB_GC_SUBWINDOW_MODE, &value);

    timespec_get(&end, TIME_UTC);
    xsr_context srctx;
    srctx.connection = connection;
    srctx.elapsed_time = end.tv_nsec - begin.tv_nsec;
    srctx.pts = 0;
    srctx.screen = screen;
    srctx.avctx = avctx;
    srctx.frame = frame;
    srctx.packet = packet;
    srctx.file = file;

    XsrStartRecording(&srctx);

    avcodec_free_context(&avctx);
    av_frame_free(&frame);
    av_packet_free(&packet);

    avcodec_close(avctx);
    
    fclose(file);
    printf("writen to %s\n", filename);

    xcb_disconnect(connection);


    return 0;
}

