#pragma once

/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <vector>

namespace examples
{
/**
 * Basic example of low-pass filter
 */
struct Highpass
{
  static constexpr auto name() { return "Highpass"; }
  static constexpr auto c_name() { return "ngry_highpass"; }
  static constexpr auto uuid() { return "1ae4129b-5598-41db-8b78-ef0756e9da1e"; }

  struct titi
  {
    /** A special case for audio processors with ports is:
     *  if the processor's first audio ports are dynamic
     *  (that is, "channels" is not a mutable thing), then they will
     *  be assigned the same amount of channels.
     *  That is, they are treated implicitly as mimicking.
     *
     *  See the sidechain example.
     */
    struct
    {
      static constexpr auto name() { return "Input"; }
      const double** samples;
      int channels;
    } audio;

    struct toto
    {
      static constexpr auto name() { return "Weight"; }
      struct widget { static inline int knob; };
      struct range
      {
        const float min = 0.;
        const float max = 1.;
        const float init = 0.5;
      };

      float value;
    } weight;
  } inputs;

  struct
  {
    struct
    {
      static constexpr auto name() { return "Output"; }
      double** samples;
      int channels;
    } audio;
  } outputs;

  // Here we have some state which depends on the host configuration (number of channels, etc).
  std::vector<float> previous_values{};

  struct setup
  {
    int channels{};
    float rate{};
  };

  // One can add such a method to recompute internal state, buffers...
  // It will be called whenever any of the values here change.
  void prepare(setup info)
  {
    // Note that the host will detect which members are available in
    // your setup struct: objects which only need channels may pass a
    // struct with only that member as argument.
    previous_values.resize(info.channels);
  }

  // Do our processing for N samples
  void operator()(int N)
  {
    const auto weight = inputs.weight.value;
    auto& p1 = inputs.audio;
    auto& p2 = outputs.audio;

    const auto chans = p1.channels;

    // Process the input buffer
    for (int i = 0; i < chans; i++)
    {
      auto& in = p1.samples[i];
      auto& out = p2.samples[i];
      float& prev = this->previous_values[i];
      out[0] = prev;
      for(int j = 1; j < N; ++j)
      {
          out[j] = weight * prev + weight * (in[j]-in[j-1]);
          prev = out[j];
      }
    }
  }
};
}

/*
auto& in = p1.samples[i];
auto& out = p2.samples[i];
float& prev = this->previous_values[i];
out[0] = in[0];
for(int j = 1; j < N; ++j)
{
    out[j] = hf_weight * prev + hf_weight * (in[j]-in[j-1]);
    prev = out[j];
}
        */
