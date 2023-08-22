#pragma once
#include <Gfx/Graph/decoders/GPUVideoDecoder.hpp>
extern "C" {
#include <libavformat/avformat.h>
}

namespace score::gfx
{

/**
 * @brief Decodes YUV422 videos.
 *
 * Adapted from YUV420 Roxlu code and
 * softpixel.com/~cwright/programming/colorspace/yuv
 */

struct YUV422P10Decoder : GPUVideoDecoder
{
  static const constexpr auto yuv422_filter = R"_(#version 450

layout(std140, binding = 0) uniform renderer_t {
mat4 clipSpaceCorrMatrix;
vec2 texcoordAdjust;

vec2 renderSize;
} renderer;

layout(binding=3) uniform sampler2D y_tex;
layout(binding=4) uniform sampler2D u_tex;
layout(binding=5) uniform sampler2D v_tex;

layout(location = 0) in vec2 v_texcoord;
layout(location = 0) out vec4 fragColor;

// See softpixel.com/~cwright/programming/colorspace/yuv
const vec3 offset = vec3(0., -0.5, -0.5);
const mat3 coeff = mat3(1.0   ,  1.0   , 1.0,
                        0.0   , -0.3455, 1.7790,
                        1.4075, -0.7169, 0.);
void main()
{
    float y = 64. * texture(y_tex, v_texcoord).r;
    float u = 64. * texture(u_tex, v_texcoord).r;
    float v = 64. * texture(v_tex, v_texcoord).r;

    fragColor = vec4(coeff * (vec3(y,u,v) + offset), 1);
}
)_";

  explicit YUV422P10Decoder(Video::ImageFormat& d)
      : decoder{d}
  {
  }

  Video::ImageFormat& decoder;
  std::pair<QShader, QShader> init(RenderList& r) override
  {
    auto& rhi = *r.state.rhi;

    const auto w = decoder.width, h = decoder.height;
    // Y
    {
      auto tex = rhi.newTexture(QRhiTexture::R16, {w, h}, 1, QRhiTexture::Flag{});
      tex->create();

      auto sampler = rhi.newSampler(
          QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
      sampler->create();
      samplers.push_back({sampler, tex});
    }

    // U
    {
      auto tex = rhi.newTexture(QRhiTexture::R16, {w / 2, h}, 1, QRhiTexture::Flag{});
      tex->create();

      auto sampler = rhi.newSampler(
          QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
      sampler->create();
      samplers.push_back({sampler, tex});
    }

    // V
    {
      auto tex = rhi.newTexture(QRhiTexture::R16, {w / 2, h}, 1, QRhiTexture::Flag{});
      tex->create();

      auto sampler = rhi.newSampler(
          QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
      sampler->create();
      samplers.push_back({sampler, tex});
    }

    return score::gfx::makeShaders(r.state, vertexShader(), yuv422_filter);
  }

  void exec(RenderList&, QRhiResourceUpdateBatch& res, AVFrame& frame) override
  {
    setYPixels(res, frame.data[0], frame.linesize[0]);
    setUPixels(res, frame.data[1], frame.linesize[1]);
    setVPixels(res, frame.data[2], frame.linesize[2]);
  }

  void
  setYPixels(QRhiResourceUpdateBatch& res, uint8_t* pixels, int stride) const noexcept
  {
    const auto w = decoder.width, h = decoder.height;
    auto y_tex = samplers[0].texture;

    QRhiTextureUploadEntry entry{0, 0, createTextureUpload(pixels, w, h, 2, stride)};

    QRhiTextureUploadDescription desc{entry};
    res.uploadTexture(y_tex, desc);
  }

  void
  setUPixels(QRhiResourceUpdateBatch& res, uint8_t* pixels, int stride) const noexcept
  {
    const auto w = decoder.width / 2, h = decoder.height;
    auto u_tex = samplers[1].texture;

    QRhiTextureUploadEntry entry{0, 0, createTextureUpload(pixels, w, h, 2, stride)};

    QRhiTextureUploadDescription desc{entry};

    res.uploadTexture(u_tex, desc);
  }

  void
  setVPixels(QRhiResourceUpdateBatch& res, uint8_t* pixels, int stride) const noexcept
  {
    const auto w = decoder.width / 2, h = decoder.height;
    auto v_tex = samplers[2].texture;

    QRhiTextureUploadEntry entry{0, 0, createTextureUpload(pixels, w, h, 2, stride)};

    QRhiTextureUploadDescription desc{entry};
    res.uploadTexture(v_tex, desc);
  }
};

}
