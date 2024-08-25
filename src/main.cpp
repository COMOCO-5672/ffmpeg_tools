#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

struct EncoderInfo {
    std::string name;
    AVCodecID codec_id;
    AVHWDeviceType hw_type;
    double performance;
};

std::vector<EncoderInfo> detect_hardware_encoders() {
    std::vector<EncoderInfo> encoders;
    AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;
    
    while ((hw_type = av_hwdevice_iterate_types(hw_type)) != AV_HWDEVICE_TYPE_NONE) {
        const AVCodec *codec = nullptr;
        void *opaque = nullptr;
        
        while ((codec = av_codec_iterate(&opaque))) {
            if (!av_codec_is_encoder(codec) || codec->type != AVMEDIA_TYPE_VIDEO)
                continue;

            if (codec->capabilities & AV_CODEC_CAP_HARDWARE)
                encoders.push_back({codec->name, codec->id, hw_type, 0.0});
        }
    }

    return encoders;
}

double test_encoder_performance(const EncoderInfo& encoder) {
    // ... [性能测试代码，与之前的示例相同] ...
}

EncoderInfo find_best_encoder() {
    auto encoders = detect_hardware_encoders();

    std::cout << "Detected hardware encoders:" << std::endl;
    for (auto& encoder : encoders) {
        std::cout << "Testing " << encoder.name << " (" << av_hwdevice_get_type_name(encoder.hw_type) << ")..." << std::endl;
        encoder.performance = test_encoder_performance(encoder);
        std::cout << "Performance: " << encoder.performance << " fps" << std::endl;
    }

    auto best_encoder = std::max_element(encoders.begin(), encoders.end(),
        [](const EncoderInfo& a, const EncoderInfo& b) { return a.performance < b.performance; });

    if (best_encoder != encoders.end()) {
        return *best_encoder;
    } else {
        return {"None", AV_CODEC_ID_NONE, AV_HWDEVICE_TYPE_NONE, 0.0};
    }
}

int main() {
    avcodec_register_all();

    EncoderInfo best = find_best_encoder();

    if (best.codec_id != AV_CODEC_ID_NONE) {
        std::cout << "\nBest encoder: " << best.name 
                  << " (" << av_hwdevice_get_type_name(best.hw_type) << ")"
                  << " with performance " << best.performance << " fps" << std::endl;
    } else {
        std::cout << "No hardware encoders found." << std::endl;
    }

    return 0;
}