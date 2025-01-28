#ifndef XSR_FORMAT_H
#define XSR_FORMAT_H

#include <xcb/xcb_image.h>
#include <libavcodec/avcodec.h>

void XsrFromatFrame(xcb_image_t *image, AVFrame *frame);

#endif
