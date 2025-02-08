#define _XOPEN_SOURCE 700
#include <xsr/xscreenrec.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

#define XSR_THREAD_MAXCOUNT 16

static int recording = 1;
static const uint8_t endcode[] = { 0, 0, 1, 0xb7 };

static pthread_mutex_t pts_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t image_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t recording_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int cond_val = 0;

static pthread_t threads[XSR_THREAD_MAXCOUNT];
static pthread_t clock_thread;


static xcb_image_t *image = NULL;

void XsrEncodeFrames(xsr_context *srctx);
void XsrEncode(AVCodecContext *avctx, AVFrame *frame, AVPacket *packet, FILE *file);
void XsrFormatVideo(xsr_context *srctx);

void *XsrClock(void *arg)
{
    struct timespec ts, rem;
    ts.tv_sec = 0;
    xsr_option fps = XsrGetOption(XSR_OPT_FRAMERATE);
    ts.tv_nsec = 1000000000l / (long)fps.framerate.framerate.num;
    while (true)
    {
        int status = nanosleep(&ts, NULL);
        pthread_mutex_lock(&recording_mutex);
        if (!recording)
        {
            return NULL;
        }
        pthread_mutex_unlock(&recording_mutex);

        pthread_cond_signal(&cond);
    }
}

void *XsrThreadCallback(void *srctxvp)
{   
    while (true)
    {
        pthread_mutex_lock(&cond_mutex);
        cond_val++;
        pthread_cond_wait(&cond, &cond_mutex);
        cond_val--;
        pthread_mutex_unlock(&cond_mutex);

        pthread_mutex_lock(&recording_mutex);
        if (!recording)
        {
            return NULL;
        }
        pthread_mutex_unlock(&recording_mutex);

        XsrEncodeFrames((xsr_context*)srctxvp);
    }   
}

void XsrStartRecording(xsr_context *srctx)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED, NULL);
    xcb_connection_t *connection = srctx->connection;
    xcb_screen_t *screen = srctx->screen;
    xcb_window_t window = screen->root;
    image = xcb_image_get(connection, window, 0, 0, screen->width_in_pixels, screen->height_in_pixels, 0x00ffffff, XCB_IMAGE_FORMAT_Z_PIXMAP);

    for (size_t i = 0; i < 2; i++)

    {
        pthread_create(&threads[i], NULL, XsrThreadCallback, srctx);
    }

    pthread_create(&clock_thread, NULL, XsrClock, NULL);

    getchar();

    pthread_mutex_lock(&recording_mutex);
    recording = false;
    pthread_mutex_unlock(&recording_mutex);

    pthread_join(clock_thread, NULL);

    while (true)
    {
        pthread_mutex_lock(&cond_mutex);
        if (cond_val == 2)
        {
            pthread_mutex_unlock(&cond_mutex);
            break;
        }
        pthread_mutex_unlock(&cond_mutex);
    }
    

    for (size_t i = 0; i < 2; i++)
    {
        pthread_cancel(threads[i]);
    }

    if (image != NULL)
    {
        xcb_image_destroy(image);
    }

    pthread_mutex_destroy(&recording_mutex);
    pthread_mutex_destroy(&cond_mutex);
    pthread_cond_destroy(&cond);
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

void XsrEncodeFrames(xsr_context *srctx)
{
    xcb_connection_t *connection = srctx->connection;
    xcb_screen_t *screen = srctx->screen;
    xcb_window_t window = screen->root;
    volatile int status = 0;
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
    srctx->frame_count = srctx->frame->pts = srctx->pts++;
    
    XsrEncode(srctx->avctx, srctx->frame, srctx->packet, srctx->file);
    pthread_mutex_unlock(&pts_mutex);
}

