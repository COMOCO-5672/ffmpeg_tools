#include "encoders_info.h"
#include <algorithm>
#include <chrono>
#include <iostream>

#define TEST_FRAMES 30
#define TEST_WIDTH 1920
#define TEST_HEIGHT 1080

namespace CODEC_INFO
{
EncodersInfo::EncodersInfo() {}

EncodersInfo::~EncodersInfo() {}

std::vector<std::tuple<std::string, AVCodecID>> EncodersInfo::GetAllEncoders(AVMediaType media_type)
{
    std::vector<std::tuple<std::string, AVCodecID>> encoders;

    if (media_type == AVMediaType::AVMEDIA_TYPE_UNKNOWN)
        return encoders;

    const AVCodec *codec = nullptr;
    void *opaque = nullptr;

    while ((codec = av_codec_iterate(&opaque))) {
        if (!av_codec_is_encoder(codec) || codec->type != media_type)
            continue;

        encoders.emplace_back(std::pair { codec->name, codec->id });
    }
    return encoders;
}

std::vector<std::tuple<std::string, AVCodecID>> EncodersInfo::GetHwEncoders(AVMediaType media_type)
{
    std::vector<std::tuple<std::string, AVCodecID>> encoders;

    if (media_type == AVMediaType::AVMEDIA_TYPE_UNKNOWN)
        return encoders;

    AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;

    while ((hw_type = av_hwdevice_iterate_types(hw_type)) != AV_HWDEVICE_TYPE_NONE) {
        const AVCodec *codec = nullptr;
        void *opaque = nullptr;

        while ((codec = av_codec_iterate(&opaque))) {
            if (!av_codec_is_encoder(codec) || codec->type != media_type)
                continue;

            if (codec->capabilities & AV_CODEC_CAP_HARDWARE)
                encoders.emplace_back(std::pair { codec->name, codec->id });
        }
    }
    return encoders;
}

std::vector<std::tuple<std::string, AVCodecID>> EncodersInfo::GetSwEncoders(AVMediaType media_type)
{

    std::vector<std::tuple<std::string, AVCodecID>> encoders;

    if (media_type == AVMediaType::AVMEDIA_TYPE_UNKNOWN)
        return encoders;

    const AVCodec *codec = nullptr;
    void *opaque = nullptr;

    while ((codec = av_codec_iterate(&opaque))) {
        if (!av_codec_is_encoder(codec) || codec->type != media_type)
            continue;

        if (~(codec->capabilities & AV_CODEC_CAP_HARDWARE))
            encoders.emplace_back(std::pair { codec->name, codec->id });
    }
    return encoders;
}

std::vector<std::tuple<std::string, AVCodecID>> EncodersInfo::GetHwEncoders(AVHWDeviceType hw_type)
{
    std::vector<std::tuple<std::string, AVCodecID>> encoders;

    if (hw_type == AVHWDeviceType::AV_HWDEVICE_TYPE_NONE)
        return encoders;

    while ((hw_type = av_hwdevice_iterate_types(hw_type)) == hw_type) {
        const AVCodec *codec = nullptr;
        void *opaque = nullptr;

        while ((codec = av_codec_iterate(&opaque))) {
            if (!av_codec_is_encoder(codec))
                continue;

            if (codec->capabilities & AV_CODEC_CAP_HARDWARE)
                encoders.emplace_back(std::pair { codec->name, codec->id });
        }
    }
    return encoders;
}

std::vector<CODEC_INFO::CodecPerformance>
EncodersInfo::DetectHwVideoEncoders(CODEC_INFO::MEDIA_TYPE media_type)
{
    std::vector<CODEC_INFO::CodecPerformance> encoders;
    const auto hw_device = GetHwEncoders(AVMediaType::AVMEDIA_TYPE_VIDEO);
    for (const auto &item : hw_device) {
        const auto name = std::get<0>(item);
        const auto codec_id = std::get<1>(item);

        std::cout << "Testing encoder:" << name << std::endl;
        CODEC_INFO::CodecPerformance encoder;
        encoder.codec_id = codec_id;
        encoder.name = name;
        encoder.performance = test_encoder_performance(name, media_type);
        std::cout << "Performance: " << encoder.performance << " fps" << std::endl;
        encoders.emplace_back(encoder);
    }
    return encoders;
}

bool EncodersInfo::FindBestHwVideoEncoder(CODEC_INFO::MEDIA_TYPE media_type,
                                          CODEC_INFO::CodecPerformance &find_codec_info)
{
    std::vector<CODEC_INFO::CodecPerformance> list = DetectHwVideoEncoders(media_type);

    const auto best_encoder = std::max_element(
        list.begin(),
        list.end(),
        [](const CODEC_INFO::CodecPerformance &a, const CODEC_INFO::CodecPerformance &b)
        { return a.performance < b.performance; });

    bool find = false;
    if (best_encoder == list.end() || best_encoder->codec_id == AV_CODEC_ID_NONE) {
        find = false;
    }
    else {
        find = true;
        find_codec_info = *best_encoder;
    }
    return find;
}

double EncodersInfo::test_encoder_performance(std::string name, CODEC_INFO::MEDIA_TYPE media_type)
{
    AVCodec *codec = avcodec_find_encoder_by_name(name.c_str());
    if (!codec)
        return 0.0;

    AVCodecContext *c = avcodec_alloc_context3(codec);
    if (!c)
        return 0.0;

    c->bit_rate = 5000000;
    c->width = TEST_WIDTH;
    c->height = TEST_HEIGHT;
    c->time_base = { 1, TEST_FRAMES };
    c->framerate = { TEST_FRAMES, 1 };
    c->gop_size = TEST_FRAMES;
    c->max_b_frames = 0;
    c->pix_fmt = media_type == CODEC_INFO::MEDIA_TYPE::HDR ? AV_PIX_FMT_YUV420P10LE
                                                           : AV_PIX_FMT_YUV420P;

    const bool is_hdr = media_type == CODEC_INFO::MEDIA_TYPE::HDR;
    if (media_type != CODEC_INFO::MEDIA_TYPE::NONE) {
        if (is_hdr) {
            c->color_primaries = AVCOL_PRI_BT2020;
            c->color_trc = AVCOL_TRC_SMPTE2084;
            c->colorspace = AVCOL_SPC_BT2020_NCL;
        }
        else {
            c->color_primaries = AVCOL_PRI_BT709;
            c->color_trc = AVCOL_TRC_BT709;
            c->colorspace = AVCOL_SPC_BT709;
        }
    }

    if (avcodec_open2(c, codec, NULL) < 0) {
        avcodec_free_context(&c);
        return 0.0;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        avcodec_free_context(&c);
        return 0.0;
    }

    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;
    frame->color_primaries = c->color_primaries;
    frame->color_trc = c->color_trc;
    frame->colorspace = c->colorspace;
    av_frame_get_buffer(frame, 0);

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        av_frame_free(&frame);
        avcodec_free_context(&c);
        return 0.0;
    }

    const auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < TEST_FRAMES; i++) {
        int max_value = is_hdr ? 1023 : 255;
        for (int y = 0; y < c->height; y++) {
            for (int x = 0; x < c->width; x++) {
                if (is_hdr) {
                    ((uint16_t *)frame->data[0])[y * frame->linesize[0] / 2 + x] =
                        ((x + y + i * 3) * 4) & max_value;
                }
                else {
                    frame->data[0][y * frame->linesize[0] + x] = (x + y + i * 3) & max_value;
                }
            }
        }
        for (int y = 0; y < c->height / 2; y++) {
            for (int x = 0; x < c->width / 2; x++) {
                if (is_hdr) {
                    ((uint16_t *)frame->data[1])[y * frame->linesize[1] / 2 + x] =
                        ((512 + y + i * 2) * 4) & max_value;
                    ((uint16_t *)frame->data[2])[y * frame->linesize[2] / 2 + x] =
                        ((256 + x + i * 5) * 4) & max_value;
                }
                else {
                    frame->data[1][y * frame->linesize[1] + x] = (128 + y + i * 2) & max_value;
                    frame->data[2][y * frame->linesize[2] + x] = (64 + x + i * 5) & max_value;
                }
            }
        }

        frame->pts = i;

        int ret = avcodec_send_frame(c, frame);
        if (ret < 0)
            break;

        while (ret >= 0) {
            ret = avcodec_receive_packet(c, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                break;
            av_packet_unref(pkt);
        }
    }

    const auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&c);

    return TEST_FRAMES / diff.count();
}

} // namespace CODEC_INFO