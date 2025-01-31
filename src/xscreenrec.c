#define _XOPEN_SOURCE 700
#include <xsr/xscreenrec.h>
#include <signal.h>


static int recording = 1;
static pthread_t listener;
static const uint8_t endcode[] = { 0, 0, 1, 0xb7 };

static timer_t timer;
static pthread_mutex_t pts_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t image_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t active_thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t active_thread_count = 0;

static xcb_image_t *image = NULL;

void *wait_end_of_recording(void *arg)
{
    getchar();

    timer_delete(timer);

    int loop = 1;
    while (loop)
    {
        pthread_mutex_lock(&active_thread_count_mutex);
        if (active_thread_count == 0)
        {
            loop = 0;
        }
        pthread_mutex_unlock(&active_thread_count_mutex);
    }

    return NULL;
}

void XsrEncodeFrames(__sigval_t __srctx);
void XsrEncode(AVCodecContext *avctx, AVFrame *frame, AVPacket *packet, FILE *file);

void XsrStartRecording(xsr_context *srctx)
{
    xcb_connection_t *connection = srctx->connection;
    xcb_screen_t *screen = srctx->screen;
    xcb_window_t window = screen->root;
    image = xcb_image_get(connection, window, 0, 0, screen->width_in_pixels, screen->height_in_pixels, 0x00ffffff, XCB_IMAGE_FORMAT_Z_PIXMAP);

    

    struct sigevent se;
    se.sigev_signo = SIGUSR1;
    se.sigev_notify = SIGEV_THREAD;
    se._sigev_un._sigev_thread._attribute = NULL;
    se._sigev_un._sigev_thread._function = XsrEncodeFrames;
    se.sigev_value.sival_ptr = srctx;
    
    timer_create(CLOCK_REALTIME, &se, &timer);

    pthread_create(&listener, NULL, wait_end_of_recording, NULL);

    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1000000000 / srctx->avctx->framerate.num;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    timer_settime(timer, 0, &its, NULL);

    pthread_join(listener, NULL);
    
    if (image != NULL)
    {
        xcb_image_destroy(image);
    }
    pthread_mutex_destroy(&pts_mutex);
    pthread_mutex_destroy(&image_mutex);
    XsrEncode(srctx->avctx, NULL, srctx->packet, srctx->file);
    fwrite(endcode, 1, 4, srctx->file);
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

void XsrEncodeFrames(__sigval_t __srctx)
{
    pthread_mutex_lock(&active_thread_count_mutex);
    active_thread_count++;
    pthread_mutex_unlock(&active_thread_count_mutex);

    xsr_context *srctx = (xsr_context*)__srctx.sival_ptr;
    xcb_connection_t *connection = srctx->connection;
    xcb_screen_t *screen = srctx->screen;
    xcb_window_t window = screen->root;
    int status = 0;
    status = pthread_mutex_trylock(&image_mutex);
    if (status == 0)
    {
        xcb_image_destroy(image);
        image = xcb_image_get(connection, window, 0, 0, screen->width_in_pixels, screen->height_in_pixels, 0x00ffffff, XCB_IMAGE_FORMAT_Z_PIXMAP);
    }
    
    pthread_mutex_lock(&pts_mutex);
    status = av_frame_make_writable(srctx->frame);
    if (status < 0)
    {
        exit(1);
    }
    
    XsrFromatFrame(image, srctx->frame);
    pthread_mutex_unlock(&image_mutex);
    srctx->frame->pts = srctx->pts++;
    XsrEncode(srctx->avctx, srctx->frame, srctx->packet, srctx->file);
    pthread_mutex_unlock(&pts_mutex);

    pthread_mutex_lock(&active_thread_count_mutex);
    active_thread_count--;
    pthread_mutex_unlock(&active_thread_count_mutex);
}

