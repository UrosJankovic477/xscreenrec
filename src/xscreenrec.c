#include <xscreenrec.h>

static const float coeffs[3][3] = 
{
    {0.299, 0.587, 0.114},
    {-0.168736, -0.331264, 0.5},
    {0.5, -0.418688, -0.081312}
};

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

AVCodecContext *XsrCreateAVCodecContext(AVCodec *codec, xcb_screen_t *screen)
{
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
    int frame_count = (int)(avctx->framerate.num * srctx->elapsed_time);
    if (frame_count <= 0)
    {
        return;
    }
    srctx->elapsed_time = 0;
    xcb_image_t *image;
    xcb_connection_t *connection = srctx->connection;
    xcb_screen_t *screen = srctx->screen;
    xcb_window_t window = screen->root;
    int status = 0;
    for (int i = 0; i < frame_count; i++)
    {
        //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin);
        image = xcb_image_get(connection, window, 0, 0, screen->width_in_pixels, screen->height_in_pixels, 0x00ffffff, XCB_IMAGE_FORMAT_Z_PIXMAP);
        //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

        //elapsed = get_elapsed_time(begin, end);
        //printf("elapsed time for xcb_image_get: %lds, %ldns\n", elapsed.tv_sec, elapsed.tv_nsec);
        
        status = av_frame_make_writable(frame);
        if (status < 0)
        {
            exit(1);
        }

        //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin);
        for (size_t y = 0; y < frame->height; y++)
        {
            for (size_t x = 0; x < frame->width; x++)
            {
                uint8_t b = image->data[(x + y * screen->width_in_pixels) * 4 + 0];
                uint8_t g = image->data[(x + y * screen->width_in_pixels) * 4 + 1];
                uint8_t r = image->data[(x + y * screen->width_in_pixels) * 4 + 2]; 
                frame->data[0][x + y * frame->linesize[0]] = (uint8_t)((uint16_t)(r * coeffs[0][0] + g * coeffs[0][1] + b * coeffs[0][2]));
            }
        }

        for (size_t y = 0; y < frame->height / 2; y++)
        {
            for (size_t x = 0; x < frame->width / 2; x++)
            {
                uint8_t b = image->data[(x + y * screen->width_in_pixels) * 8 + 0];
                uint8_t g = image->data[(x + y * screen->width_in_pixels) * 8 + 1];
                uint8_t r = image->data[(x + y * screen->width_in_pixels) * 8 + 2]; 
                frame->data[1][x + y * frame->linesize[1]] = (uint8_t)(128 + (uint16_t)(r * coeffs[1][0] + g * coeffs[1][1] + b * coeffs[1][2]));
                frame->data[2][x + y * frame->linesize[2]] = (uint8_t)(128 + (uint16_t)(r * coeffs[2][0] + g * coeffs[2][1] + b * coeffs[2][2]));
            }
            
        }

        frame->pts = srctx->pts++;
        XsrEncode(avctx, frame, packet, file);
        xcb_image_destroy(image);

    }
}

