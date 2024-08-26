#pragma once

#include "ff_include.h"
#include <string>

namespace CODEC_INFO {

enum class MEDIA_TYPE { NONE, SDR, HDR };
struct CodecPerformance {
    std::string name;
    AVCodecID codec_id;
    AVHWDeviceType hw_type;
    double performance;
};

} // namespace CODEC_INFO