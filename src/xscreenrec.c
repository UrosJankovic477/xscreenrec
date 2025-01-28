#include <xsr/xscreenrec.h>

static int recording = 1;
static pthread_t listener;
static const uint8_t endcode[] = { 0, 0, 1, 0xb7 };

void *wait_end_of_recording(void *arg)
{
    recording = 1;
    getchar();
    recording = 0;
    return NULL;
}

void XsrStartRecording(xsr_context *srctx, AVCodecContext *avctx, AVFrame *frame, AVPacket *packet, FILE *file)
{
    struct timespec begin, end;
    
    pthread_create(&listener, NULL, wait_end_of_recording, NULL);
    while (recording)
    {
        timespec_get(&begin, TIME_UTC);
        XsrEncodeFrames(srctx, avctx, frame, packet, file); 
        timespec_get(&end, TIME_UTC);
        srctx->elapsed_time += end.tv_sec - begin.tv_sec + ((double)(end.tv_nsec - begin.tv_nsec)) / 1000000000.0;
    }
    pthread_join(listener, NULL);
    
    XsrEncode(avctx, NULL, packet, file);
    fwrite(endcode, 1, 4, file);
}

void XsrEncode(AVCodecContext *avctx, AVFrame *frame, AVPacket *packet, FILE *file)
{
    int status = avcodec_send_frame(avctx, frame);
    if (status < 0)
    {
        fprintf(stderr, "failed to send frame %ld\n", frame->pts);
        exit(1);
    }
    while (status >= 0)
    {
        status = avcodec_receive_packet(avctx, packet);
        if (status == AVERROR(EAGAIN) || status == AVERROR_EOF)
        {
            return;
        }
        else if (status < 0)
        {
            fprintf(stderr, "failed to recieve packet %ld\n", packet->pts);
            exit(1);
        }
        fwrite(packet->data, 1, packet->size, file);
        av_packet_unref(packet);
    }
}

void XsrEncodeFrames(xsr_context *srctx, AVCodecContext *avctx, AVFrame *frame, AVPacket *packet, FILE *file)
{
    int frame_count = floor(avctx->framerate.num * srctx->elapsed_time);
    if (frame_count <= 0)
    {
        return;
    }
    srctx->elapsed_time -= (double)frame_count / (double)avctx->framerate.num;
    xcb_image_t *image;
    xcb_connection_t *connection = srctx->connection;
    xcb_screen_t *screen = srctx->screen;
    xcb_window_t window = screen->root;
    int status = 0;
    for (; frame_count >= 1; frame_count--)
    {
        image = xcb_image_get(connection, window, 0, 0, screen->width_in_pixels, screen->height_in_pixels, 0x00ffffff, XCB_IMAGE_FORMAT_Z_PIXMAP);

        status = av_frame_make_writable(frame);
        if (status < 0)
        {
            exit(1);
        }
        
        XsrFromatFrame(image, frame);

        frame->pts = srctx->pts++;
        XsrEncode(avctx, frame, packet, file);
        xcb_image_destroy(image);

    }
}

