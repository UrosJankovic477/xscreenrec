#ifndef XSR_SCREENREC_H
#define XSR_SCREENREC_H

#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_image.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <time.h> 
#include <pthread.h>
#include <xsr/options.h>
#include <xsr/format.h>
#include <math.h>

typedef struct struct_xsr_context
{
    xcb_connection_t    *connection;
    xcb_screen_t        *screen;
    AVCodecContext      *avctx;
    AVFrame             *frame;
    AVPacket            *packet;
    FILE                *file;
    int64_t             pts;
    time_t              elapsed_time;
    uint32_t            frame_count;
}
xsr_context;

void XsrStartRecording(xsr_context *srctx);

#endif
