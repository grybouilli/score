#!/bin/bash -eux

source ci/common.setup.sh

PACKAGES=
if [[ ! -f /usr/lib/libjack.so ]]; then
  PACKAGES+=" jack2 "
fi
if [[ ! -f /usr/lib/libfontconfig.so ]]; then
  PACKAGES+=" fontconfig "
fi
if [[ ! -f /usr/lib/libharfbuzz.so ]]; then
  PACKAGES+=" harfbuzz "
fi
if [[ ! -f /usr/lib/libfreetype.so ]]; then
  PACKAGES+=" freetype2 "
fi

$SUDO pacman -Syyu --noconfirm
$SUDO pacman -S --noconfirm --needed \
   $PACKAGES \
   cmake ninja gcc llvm clang boost \
   icu \
   ffmpeg portaudio lv2 suil lilv sdl2 alsa-lib \
   avahi fftw bluez-libs \
   tar xz wget \
   brotli double-conversion zstd glib2 libb2 pcre2 \
   libxkbcommon libxkbcommon-x11 \
   vulkan-headers vulkan-icd-loader \
   mesa libglvnd \
   libdrm tslib udev zstd \
   xcb-proto xcb-util xcb-util-cursor xcb-util-image \
   xcb-util-keysyms xcb-util-renderutil xcb-util-wm \
   qt6-base qt6-shadertools qt6-tools qt6-5compat qt6-declarative qt6-scxml qt6-websockets qt6-serialport

source ci/common.deps.sh
