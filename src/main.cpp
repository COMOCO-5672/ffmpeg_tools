#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include "CLI11.hpp"
#include "codec_info.h"
#include "encoders_info.h"
#include "ff_include.h"

namespace parse_args {

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

    std::vector<CODEC_INFO::CodecPerformance> list =
        encoders->DetectHwVideoEncoders(parse_args::E_MEDIA_TYPE);

    auto best_encoder = std::max_element(
        list.begin(), list.end(),
        [](const CODEC_INFO::CodecPerformance &a, const CODEC_INFO::CodecPerformance &b) {
            return a.performance < b.performance;
        });

    if (best_encoder != list.end() || best_encoder->codec_id == AV_CODEC_ID_NONE) {
        std::cout << "No hardware encoders found." << std::endl;
    } else {
        std::cout << "\nBest device encoder: " << best_encoder->name << " with performance "
                  << best_encoder->performance << " fps" << std::endl;
    }

    return 0;
}