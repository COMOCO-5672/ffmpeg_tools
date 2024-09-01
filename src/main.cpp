#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include "CLI11.hpp"
#include "codec_info/codec_info.h"
#include "codec_info/decoders_info.h"
#include "codec_info/encoders_info.h"
#include "third_party/ff_include.h"

namespace parse_args
{

static CODEC_INFO::MEDIA_TYPE E_MEDIA_TYPE = CODEC_INFO::MEDIA_TYPE::NONE;

void parse_media_type(CLI::App &app)
{
    std::map<std::string, CODEC_INFO::MEDIA_TYPE> mode_map {
        { "none", CODEC_INFO::MEDIA_TYPE::NONE },
        { "sdr", CODEC_INFO::MEDIA_TYPE::SDR },
        { "hdr", CODEC_INFO::MEDIA_TYPE::HDR },
    };
    app.add_option("-m,--media_type", E_MEDIA_TYPE, "Media type (NONE, SDR, HDR)")
        ->transform(CLI::CheckedTransformer(mode_map, CLI::ignore_case));
};

void parse_options(CLI::App &app) { parse_media_type(app); }

}; // namespace parse_args

int main(int argc, char **argv)
{
    CLI::App app { "Video Tools" };
    parse_args::parse_options(app);
    CLI11_PARSE(app, argc, argv);

    std::cout << "media_type:" << static_cast<int>(parse_args::E_MEDIA_TYPE) << std::endl;

    avcodec_register_all();

    auto encoders = new CODEC_INFO::EncodersInfo();
    CODEC_INFO::CodecPerformance codec_info;
    const auto find_encoder =
        encoders->FindBestHwVideoEncoder(parse_args::E_MEDIA_TYPE, codec_info);

    if (!find_encoder) {
        std::cout << "No hardware encoders found." << std::endl;
    }
    else {
        std::cout << "\nBest device encoder: " << codec_info.name << " with performance "
                  << codec_info.performance << " fps" << std::endl;
    }
    std::cout << std::endl;
    const auto encoders_list = encoders->GetDeviceHwEncoders(AVMediaType::AVMEDIA_TYPE_VIDEO);
    for (auto &item : encoders_list) {
        std::cout << "Supported HW encoder: " << std::get<0>(item).c_str()
                  << " (Device: " << av_hwdevice_get_type_name(std::get<2>(item)) << ")"
                  << std::endl;
    }
    std::cout << std::endl;
    auto decoders = new CODEC_INFO::DecodersInfo();
    auto decoders_list = decoders->GetDeviceHwDecoders(AVMediaType::AVMEDIA_TYPE_VIDEO);
    for (auto &item : decoders_list) {
        std::cout << "Supported HW decoder: " << std::get<0>(item).c_str()
                  << " (Device: " << av_hwdevice_get_type_name(std::get<2>(item)) << ")"
                  << std::endl;
    }

    return 0;
}