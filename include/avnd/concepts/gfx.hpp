#pragma once

/* SPDX-License-Identifier: GPL-3.0-or-later OR BSL-1.0 OR CC0-1.0 OR CC-PDCC OR 0BSD */

#include <avnd/concepts/generic.hpp>
#include <avnd/common/concepts_polyfill.hpp>

namespace avnd
{

template <typename T>
concept cpu_texture = requires(T t)
{
  t.bytes;
  t.width;
  t.height;
  typename T::format;
};

template <typename T>
concept cpu_texture_port = requires (T t) {
  t.texture;
} && cpu_texture<decltype(T{}.texture)>;

template <typename T>
concept gpu_texture = requires(T t)
{
  t.handle;
  t.width;
  t.height;
  t.format;
};

template <typename T>
concept gpu_texture_port = requires (T t) {
  t.texture;
} && gpu_texture<decltype(T{}.texture)>;


template <typename T>
concept texture_port = cpu_texture_port<T> || gpu_texture_port<T>;
}

/*
struct tester {
    struct my_cpu_tex {
      unsigned char* bytes;
      int width;
      int height;
      int format;
    };
    struct my_gpu_tex {
      std::intptr_t handle;
      int width;
      int height;
      int format;
    };
    static_assert(avnd::cpu_texture<my_cpu_tex>);
    static_assert(avnd::gpu_texture<my_gpu_tex>);

    struct A {
      my_cpu_tex texture;
    };
    struct B {
      my_gpu_tex texture;
    };
    static_assert(avnd::cpu_texture_port<A>);
    static_assert(avnd::gpu_texture_port<B>);

};
*/