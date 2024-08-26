#pragma once

#include "codec_info.h"
#include <vector>

namespace CODEC_INFO
{
class EncodersInfo
{
  private:
    /* data */
  public:
    EncodersInfo();
    ~EncodersInfo();

    std::vector<std::tuple<std::string, AVCodecID>> GetAllEncoders(AVMediaType media_type);

    std::vector<std::tuple<std::string, AVCodecID>> GetHwEncoders(AVMediaType media_type);
    std::vector<std::tuple<std::string, AVCodecID>> GetSwEncoders(AVMediaType media_type);

    std::vector<std::tuple<std::string, AVCodecID>> GetHwEncoders(AVHWDeviceType hw_type);

    // enum all hw encoders and return all device perfomance
    std::vector<CODEC_INFO::CodecPerformance>
    DetectHwVideoEncoders(CODEC_INFO::MEDIA_TYPE media_type);

  private:
    double test_encoder_performance(std::string name, CODEC_INFO::MEDIA_TYPE media_type);
};
} // namespace CODEC_INFO