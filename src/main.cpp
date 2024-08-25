#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

#define TEST_FRAMES 30
#define TEST_WIDTH 1920
#define TEST_HEIGHT 1080

struct EncoderInfo {
  std::string name;
  AVCodecID codec_id;
  AVHWDeviceType hw_type;
  double performance;
};

std::vector<EncoderInfo> detect_hardware_encoders() {
  std::vector<EncoderInfo> encoders;
  AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;

  while ((hw_type = av_hwdevice_iterate_types(hw_type)) !=
         AV_HWDEVICE_TYPE_NONE) {
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

double test_encoder_performance(const EncoderInfo &encoder) {
  AVCodec *codec = avcodec_find_encoder_by_name(encoder.name.c_str());
  if (!codec) {
    return 0.0;
  }

  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  if (!ctx) {
    return 0.0;
  }

  ctx->bit_rate = 5000000;
  ctx->width = TEST_WIDTH;
  ctx->height = TEST_HEIGHT;
  ctx->time_base = {1, 30};
  ctx->framerate = {30, 1};
  ctx->gop_size = 30;
  ctx->max_b_frames = 1;
  ctx->pix_fmt = AV_PIX_FMT_YUV420P;

  if (avcodec_open2(ctx, codec, NULL) < 0) {
    avcodec_free_context(&ctx);
    return 0.0;
  }

  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    avcodec_free_context(&ctx);
    return 0.0;
  }

  frame->format = ctx->pix_fmt;
  frame->width = ctx->width;
  frame->height = ctx->height;
  av_frame_get_buffer(frame, 0);

  AVPacket *pkt = av_packet_alloc();
  if (!pkt) {
    av_frame_free(&frame);
    avcodec_free_context(&ctx);
    return 0.0;
  }

  auto start_time = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < TEST_FRAMES; ++i) {
    for (int y = 0; y < ctx->height; ++y) {
      for (int x = 0; x < ctx->width; ++x) {
        frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
      }
    }

    for (int y = 0; y < ctx->height / 2; ++y) {
      for (int x = 0; x < ctx->width / 2; ++x) {
        frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 3;
        frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 3;
      }
    }

    frame->pts = i;
    int ret = avcodec_send_frame(ctx, frame);

    if (ret < 0)
      break;

    while (ret >= 0) {
      ret = avcodec_receive_packet(ctx, pkt);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      }

      if (ret < 0)
        break;
      av_packet_unref(pkt);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                    start_time)
                  .count();

  av_frame_free(&frame);
  av_packet_free(&pkt);
  avcodec_free_context(&ctx);

  //Obtain how many frames of video can be encoded per second. 
  return TEST_FRAMES * 1.0 / diff * 1000.0;
}

EncoderInfo find_best_encoder() {
  auto encoders = detect_hardware_encoders();

  std::cout << "Detected hardware encoders:" << std::endl;
  for (auto &encoder : encoders) {
    std::cout << "Testing encoder:" << encoder.name << " ("
              << av_hwdevice_get_type_name(encoder.hw_type) << ")..."
              << std::endl;
    encoder.performance = test_encoder_performance(encoder);
    std::cout << "Performance: " << encoder.performance << " fps" << std::endl;
  }

  auto best_encoder =
      std::max_element(encoders.begin(), encoders.end(),
                       [](const EncoderInfo &a, const EncoderInfo &b) {
                         return a.performance < b.performance;
                       });

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
    std::cout << "\nBest encoder: " << best.name << " ("
              << av_hwdevice_get_type_name(best.hw_type) << ")"
              << " with performance " << best.performance << " fps"
              << std::endl;
  } else {
    std::cout << "No hardware encoders found." << std::endl;
  }

  return 0;
}