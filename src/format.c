#include <xsr/format.h>

static const float coeffs[3][3] = 
{
    {0.299, 0.587, 0.114},
    {-0.168736, -0.331264, 0.5},
    {0.5, -0.418688, -0.081312}
};

typedef enum enum_xsr_subsampling
{
    XSR_4_4_4,
    XSR_4_2_2,
    XSR_4_2_0,
    XSR_4_1_1,
    XSR_4_1_0,
}
xsr_subsampling;

void XsrFormatYUV(xcb_image_t *image, AVFrame *frame, xsr_subsampling subsampling)
{
    int vertical_subsample, horizontal_subsample;
    switch (subsampling)
    {
        case XSR_4_4_4:
        {
            vertical_subsample = 1;
            horizontal_subsample = 1;
            break;
        }
        case XSR_4_2_2:
        {
            vertical_subsample = 2;
            horizontal_subsample = 1;
            break;
        }
        case XSR_4_2_0:
        {
            vertical_subsample = 2;
            horizontal_subsample = 2;
            break;
        }
        case XSR_4_1_1:
        {
            vertical_subsample = 4;
            horizontal_subsample = 1;
            break;
        }
        case XSR_4_1_0:
        {
            vertical_subsample = 4;
            horizontal_subsample = 4;
            break;
        }
        default:
        {
            vertical_subsample = 1;
            horizontal_subsample = 1;
            break;
        }
    }

    for (size_t y = 0; y < frame->height; y++)
    {
        for (size_t x = 0; x < frame->width; x++)
        {
            uint8_t b = image->data[(x + y * frame->width) * 4];
            uint8_t g = image->data[(x + y * frame->width) * 4 + 1];
            uint8_t r = image->data[(x + y * frame->width) * 4 + 2]; 
            frame->data[0][x + y * frame->linesize[0]] = (uint8_t)((uint16_t)(r * coeffs[0][0] + g * coeffs[0][1] + b * coeffs[0][2]));
        }
    }
    for (size_t y = 0; y < frame->height / vertical_subsample; y++)
    {
        for (size_t x = 0; x < frame->width / horizontal_subsample; x++)
        {
            uint8_t b = image->data[(x * horizontal_subsample + y * vertical_subsample * frame->width) * 4];
            uint8_t g = image->data[(x * horizontal_subsample + y * vertical_subsample * frame->width) * 4 + 1];
            uint8_t r = image->data[(x * horizontal_subsample + y * vertical_subsample * frame->width) * 4 + 2]; 
            frame->data[1][x + y * frame->linesize[1]] = (uint8_t)(128 + (uint16_t)(r * coeffs[1][0] + g * coeffs[1][1] + b * coeffs[1][2]));
            frame->data[2][x + y * frame->linesize[2]] = (uint8_t)(128 + (uint16_t)(r * coeffs[2][0] + g * coeffs[2][1] + b * coeffs[2][2]));
        }
    }
}

void XsrFormatRGB(xcb_image_t *image, AVFrame *frame)
{
    for (size_t y = 0; y < frame->height; y++)
    {
        for (size_t x = 0; x < frame->width; x++)
        {
            uint8_t b = image->data[(x + y * frame->width) * 4];
            uint8_t g = image->data[(x + y * frame->width) * 4 + 1];
            uint8_t r = image->data[(x + y * frame->width) * 4 + 2]; 
            frame->data[0][x + y * frame->linesize[0]] = r;
            frame->data[0][x + y * frame->linesize[0] + 1] = b;
            frame->data[0][x + y * frame->linesize[0] + 2] = g;

        }
    }
}

void XsrFromatFrame(xcb_image_t *image, AVFrame *frame)
{
    switch (frame->format)
    {
        case AV_PIX_FMT_RGB24:
        {
            return XsrFormatRGB(image, frame);
        }
        case AV_PIX_FMT_YUV410P:
        {
            return XsrFormatYUV(image, frame, XSR_4_1_0);
        }
        case AV_PIX_FMT_YUV411P:
        {
            return XsrFormatYUV(image, frame, XSR_4_1_1);
        }
        case AV_PIX_FMT_YUV420P:
        {
            return XsrFormatYUV(image, frame, XSR_4_2_0);
        }
        case AV_PIX_FMT_YUV422P:
        {
            return XsrFormatYUV(image, frame, XSR_4_2_2);
        }
        case AV_PIX_FMT_YUV444P:
        {
            return XsrFormatYUV(image, frame, XSR_4_4_4);
        }
        default:
        {
            exit(1);
        }
    }
}

