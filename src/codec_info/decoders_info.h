#pragma once

#include "codec_info.h"
#include <vector>

namespace CODEC_INFO
{
class DecodersInfo
{
  private:
    /* data */
  public:
    DecodersInfo();
    ~DecodersInfo();

    std::vector<std::tuple<std::string, AVCodecID>> GetAllDecoders(AVMediaType media_type);

    std::vector<std::tuple<std::string, AVCodecID>> GetHwDecoders(AVMediaType media_type);
    // NOTE::Obtain the hardware devices supported by the current computer.  
    std::vector<std::tuple<std::string, AVCodecID, AVHWDeviceType>> GetDeviceHwDecoders(AVMediaType media_type);
    std::vector<std::tuple<std::string, AVCodecID>> GetSwDecoders(AVMediaType media_type);

  private:
};
} // namespace CODEC_INFO