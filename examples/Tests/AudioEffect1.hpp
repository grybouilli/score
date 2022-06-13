#pragma once
#include <avnd/concepts/audio_port.hpp>
#include <avnd/concepts/parameter.hpp>
#include <halp/audio.hpp>
#include <halp/controls.hpp>
#include <halp/meta.hpp>
#include <halp/sample_accurate_controls.hpp>

namespace examples
{
/**
 * This example exhibits a simple multi-channel effect processor.
 */
struct FirstTryEffect
{
  halp_meta(name, "First Try Audio Effect");
  halp_meta(c_name, "effect_2121");
  halp_meta(category, "Demo");
  halp_meta(author, "ngry");
  halp_meta(description, "idk");
  halp_meta(uuid, "cce940d5-c591-44bf-8c14-ba0825fac88e");

  /**
   * Here we have a special case, which happens to be the most common case in audio
   * development. If our inputs start with an audio port of the shape
   *
   *     const double** samples;
   *     std::size_t channels;
   *
   * and our outputs starts with an audio port of shape
   *
   *     double** samples;
   *     std::size_t channels;
   *
   * then it is assumed that we are writing an effect processor, where the outputs
   * should match the inputs. There will be as many output channels as input channels,
   * with enough samples allocated to write from 0 to N.
   *
   * In all the other cases, it is necessary to specify the number of channels for the output
   * as in the Sidechain example.
   */
  struct
  {
    //halp::audio_input_bus<"Main Input"> audio;
    halp::accurate<halp::val_port<"In", float>> in;
    halp::knob_f32<"Gain", halp::range{.min = 0.f, .max = 100.f, .init = 10.f}> gain;
    halp::knob_f32<"High Frequency", halp::range{.min = 0.f, .max = 100.f, .init = 50.f}> high_frequency;
  } inputs;

  struct
  {
    halp::accurate<halp::val_port<"Out", float>> out;
    //halp::audio_output_bus<"Main Output"> audio;
  } outputs;

  /** Most basic effect: multiply N samples of inputs by a gain into equivalent outputs **/
  void operator()(std::size_t N)
  {
    auto& gain = inputs.gain;
    auto& h_frq = inputs.high_frequency;
    
    for (auto& [timestamp, value] : inputs.in.values)
    {
        outputs.out.values[timestamp] = value * gain * .01f + .01f * h_frq;
    }
  }
};
}
