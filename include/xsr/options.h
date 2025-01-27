#ifndef XSR_OPTIONS_H
#define XSR_OPTIONS_H

#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>


typedef enum enum_xsr_optionid
{
    XSR_OPT_OUTPUTPATH,
    XSR_OPT_CODEC,
    XSR_OPT_FRAMERATE,
    XSR_OPT_BITRATE,
    XSR_OPT_PIXFMT,
    XSR_OPT_COUNT
}
xsr_optionid;

typedef struct struct_xsr_option_base
{
    xsr_optionid    id;
    const char      *name;
    const char      *longname;
}
xsr_option_base;

typedef struct struct_xsr_option_outputpath
{
    xsr_option_base base;
    const char      *path;    
}
xsr_option_outputpath;

typedef struct struct_xsr_option_codec
{
    xsr_option_base base;
    enum AVCodecID  codecid;
}
xsr_option_codec;

typedef struct struct_xsr_option_framerate
{
    xsr_option_base base;
    AVRational      framerate;
}
xsr_option_framerate;

typedef struct struct_xsr_option_bitrate
{
    xsr_option_base base;
    int64_t         bit_rate;
}
xsr_option_bitrate;

typedef struct struct_xsr_option_pixfmt
{
    xsr_option_base     base;
    enum AVPixelFormat  pixfmt;
}
xsr_option_pixfmt;

typedef union union_xsr_option
{
    xsr_option_base         base;
    xsr_option_outputpath   outputpath;
    xsr_option_codec        codec;
    xsr_option_framerate    framerate;
    xsr_option_bitrate      bit_rate;
    xsr_option_pixfmt       pixfmt;
}
xsr_option;

void XsrReadOptions(int argc, char const **argv);

xsr_option XsrGetOption(xsr_optionid id);

#endif