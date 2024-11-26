#include "decoders_info.h"

namespace CODEC_INFO
{
    DecodersInfo::DecodersInfo() {}

    DecodersInfo::~DecodersInfo() {}

    std::vector<std::tuple<std::string, AVCodecID>>
    DecodersInfo::GetAllDecoders(AVMediaType media_type)
    {
        std::vector<std::tuple<std::string, AVCodecID>> decoders;

        if (media_type == AVMediaType::AVMEDIA_TYPE_UNKNOWN)
            return decoders;

        const AVCodec *codec = nullptr;
        void *opaque = nullptr;

        while ((codec = av_codec_iterate(&opaque))) {
            if (!av_codec_is_decoder(codec) || codec->type != media_type)
                continue;

            decoders.emplace_back(std::pair { codec->name, codec->id });
        }
        return decoders;
    }

    std::vector<std::tuple<std::string, AVCodecID, AVHWDeviceType>>
    DecodersInfo::GetDeviceHwDecoders(AVMediaType media_type)
    {
        std::vector<std::tuple<std::string, AVCodecID, AVHWDeviceType>> supported_decoders;
        AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;

        while ((hw_type = av_hwdevice_iterate_types(hw_type)) != AV_HWDEVICE_TYPE_NONE) {
            AVBufferRef *hw_device_ctx = nullptr;
            if (av_hwdevice_ctx_create(&hw_device_ctx, hw_type, nullptr, nullptr, 0) < 0) {
                continue;
            }

            const AVCodec *codec = nullptr;
            void *opaque = nullptr;
            while ((codec = av_codec_iterate(&opaque))) {
                if (!av_codec_is_decoder(codec) || codec->type != media_type) {
                    continue;
                }

                for (int i = 0;; i++) {
                    const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
                    if (!config) {
                        break;
                    }

                    if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                        config->device_type == hw_type) {
                        AVCodecContext *ctx = avcodec_alloc_context3(codec);
                        if (!ctx) {
                            continue;
                        }

                        ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
                        if (avcodec_open2(ctx, codec, nullptr) == 0) {
                            supported_decoders.emplace_back(codec->name, codec->id, hw_type);
                        }
                        avcodec_free_context(&ctx);
                        break;
                    }
                }
            }
            av_buffer_unref(&hw_device_ctx);
        }

        return supported_decoders;
    }

    std::vector<std::tuple<std::string, AVCodecID>>
    DecodersInfo::GetHwDecoders(AVMediaType media_type)
    {
        std::vector<std::tuple<std::string, AVCodecID>> decoders;

        if (media_type == AVMediaType::AVMEDIA_TYPE_UNKNOWN)
            return decoders;

        const AVCodec *codec = nullptr;
        void *opaque = nullptr;

        auto hw_type = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;

        while ((hw_type = av_hwdevice_iterate_types(hw_type)) != AV_HWDEVICE_TYPE_NONE) {
            while ((codec = av_codec_iterate(&opaque))) {
                if (!av_codec_is_decoder(codec) || codec->type != media_type)
                    continue;

                if (codec->capabilities & AV_CODEC_CAP_HARDWARE) {
                    decoders.emplace_back(std::pair { codec->name, codec->id });
                }
            }
        }
        return decoders;
    }
    std::vector<std::tuple<std::string, AVCodecID>>
    DecodersInfo::GetSwDecoders(AVMediaType media_type)
    {
        std::vector<std::tuple<std::string, AVCodecID>> decoders;

        if (media_type == AVMediaType::AVMEDIA_TYPE_UNKNOWN)
            return decoders;

        const AVCodec *codec = nullptr;
        void *opaque = nullptr;

        while ((codec = av_codec_iterate(&opaque))) {
            if (!av_codec_is_decoder(codec) || codec->type != media_type)
                continue;

            if (~(codec->capabilities & AV_CODEC_CAP_HARDWARE)) {
                decoders.emplace_back(std::pair { codec->name, codec->id });
            }
        }
        return decoders;
    }

} // namespace CODEC_INFO
