#include <xsr/options.h>
#include <string.h>

static xsr_option options[XSR_OPT_COUNT] =
{
    {.outputpath = {(xsr_option_base){XSR_OPT_OUTPUTPATH, "-o", "--output-path"}, NULL}},
    {.codec = {(xsr_option_base){XSR_OPT_CODEC, "-c", "--codec"}, AV_CODEC_ID_H264}},
    {.framerate = {(xsr_option_base){XSR_OPT_FRAMERATE, "-f", "--framerate"}, (AVRational){25, 1}}},
    {.bit_rate = {(xsr_option_base){XSR_OPT_BITRATE, "-b", "--bitrate"}, 1000000}},
    {.pixfmt = {(xsr_option_base){XSR_OPT_PIXFMT, "-p", "--pixel-fmt"}, AV_PIX_FMT_YUV420P}},
};

xsr_optionid XsrFindOptionByName(const char *name)
{
    xsr_optionid i = 0;
    for (i = 0; i < XSR_OPT_COUNT && strcmp(name, options[i].base.name) != 0 &&  strcmp(name, options[i].base.longname); i++);
    return i;
}

void XsrReadOptions(int argc, char const **argv)
{
    

    options[XSR_OPT_OUTPUTPATH].base = (xsr_option_base){XSR_OPT_OUTPUTPATH, "-o", "--output-path"};
    options[XSR_OPT_CODEC].base = (xsr_option_base){XSR_OPT_CODEC, "-c", "--codec"};


    for (int i = 1; i < argc; i++)
    {
        xsr_optionid optionid = XsrFindOptionByName(argv[i]);
        switch (optionid)
        {
            case XSR_OPT_OUTPUTPATH:
            {
                i++;
                options[optionid].outputpath.path = argv[i];
                break;
            }
            case XSR_OPT_CODEC:
            {
                i++;
                const AVCodec *codec = avcodec_find_encoder_by_name(argv[i]);
                if (codec == NULL)
                {
                    fprintf(stderr, "Couldn't find codec %s\n", argv[i]);
                    exit(1);
                }
                options[optionid].codec.codecid = codec->id;
                
                break;
            }
            case XSR_OPT_FRAMERATE:
            {
                i++;
                options[optionid].framerate.framerate = (AVRational){atoi(argv[i]), 1};
                break;
            }
            case XSR_OPT_BITRATE:
            {
                i++;
                options[optionid].bit_rate.bit_rate = atol(argv[i]);
                break;
            }
            case XSR_OPT_PIXFMT:
            {
                i++;
                const char *color_space = argv[i];
                if (strcmp(color_space, "yuv") == 0)
                {
                    i++;
                    int grouping = atoi(argv[i]);
                    switch (grouping)
                    {
                        case 444:
                        {
                            options[optionid].pixfmt.pixfmt = AV_PIX_FMT_YUV444P;
                            break;
                        }
                        case 422:
                        {
                            options[optionid].pixfmt.pixfmt = AV_PIX_FMT_YUV422P;
                            break;
                        }
                        case 420:
                        {
                            options[optionid].pixfmt.pixfmt = AV_PIX_FMT_YUV420P;
                            break;
                        }
                        case 411:
                        {
                            options[optionid].pixfmt.pixfmt = AV_PIX_FMT_YUV411P;
                            break;
                        }
                        case 410:
                        {
                            options[optionid].pixfmt.pixfmt = AV_PIX_FMT_YUV410P;
                            break;
                        }

                        default:
                        {
                            fprintf(stderr, "Unrecognized pixel format. Recognized YUV formats: YUV444P, YUV422P, YUV420P, YUV411P, YUV410P\n");
                            exit(1);
                        }
                    }
                }
                else if (strcmp(color_space, "rgb"))
                {
                    options[optionid].pixfmt.pixfmt = AV_PIX_FMT_RGB24;
                }
                
                break;
            }
            default:
            {
                fprintf(stderr, "Unrecognized option %s\n", argv[i]);
                exit(1);
                break;
            }
        }
    }
    
}

xsr_option XsrGetOption(xsr_optionid id)
{
    if (id >= XSR_OPT_COUNT || id < 0)
    {
        fprintf(stderr, "Invalid option id used");
        exit(1);
    }
    
    return options[id];
}