#!/bin/bash

if [[ $# > 0 ]]; then
  export SDK_VERSION=$1
else
  export SDK_VERSION=sdk28
fi

echo "Running on OSTYPE: '$OSTYPE'"
echo "Using SDK: $SDK_VERSION"
export LATEST_TAG=$(git describe --tags --abbrev=0)
export LATEST_TAG_NOV=$(echo "$LATEST_TAG" | sed "s/v//")
export BASE_SDK=https://github.com/ossia/score-sdk/releases/download/$SDK_VERSION
export BOOST_SDK=https://github.com/ossia/score-sdk/releases/download/sdk28
export BOOST_VER=boost_1_82_0
export LATEST_RELEASE=https://github.com/ossia/score/releases/download/$LATEST_TAG

if [[ "$OSTYPE" == "darwin"* ]]; then
(
  # First download the compiler and base libraries
  SDK_DIR=/opt/ossia-sdk-x86_64
  sudo mkdir -p "$SDK_DIR"
  sudo chown -R $(whoami) /opt
  sudo chmod -R a+rwx /opt

  (
    SDK_ARCHIVE=sdk-macOS.tar.gz
    wget -nv "$BASE_SDK/$SDK_ARCHIVE" -O "$SDK_ARCHIVE"
    tar -xzf "$SDK_ARCHIVE" --strip-components=2 --directory "$SDK_DIR"
    rm -rf "$SDK_ARCHIVE"
  )

  # Download boost
  (
    BOOST="$BOOST_VER.tar.gz"
    wget -nv "$BOOST_SDK/$BOOST" -O "$BOOST"
    mkdir -p "$SDK_DIR/boost/include"
    tar -xzf "$BOOST" --strip-components=1 --directory "$SDK_DIR/boost/include"
  )

  ls "$SDK_DIR"
)
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
(
  # First download the compiler and base libraries
  SDK_DIR=/opt/ossia-sdk
  sudo mkdir -p "$SDK_DIR"
  sudo chown -R $(whoami) "$SDK_DIR"

  (
    SDK_ARCHIVE=sdk-linux.tar.xz
    wget -nv "$BASE_SDK/$SDK_ARCHIVE" -O "$SDK_ARCHIVE"
    tar xaf "$SDK_ARCHIVE" --strip-components=2 --directory "$SDK_DIR"
    rm -rf "$SDK_ARCHIVE"
  )
  ls "$SDK_DIR"/

  # Download boost
  (
    BOOST="$BOOST_VER.tar.gz"
    wget -nv "$BOOST_SDK/$BOOST" -O "$BOOST"
    mkdir -p "$SDK_DIR/boost/include"
    tar -xzf "$BOOST" --strip-components=1 --directory "$SDK_DIR/boost/include"
  )

  ls "$SDK_DIR"
)
else
(
  # First download the compiler and base libraries
  SDK_DIR=/c/ossia-sdk
  mkdir "$SDK_DIR"
  cd "$SDK_DIR"

  # Download cmake
  if [[ ! -d "$SDK_DIR/cmake" ]] ; then
    if ! command -v cmake >/dev/null 2>&1 ; then
      echo "CMake not found, installing..."
      curl -L -0 https://github.com/Kitware/CMake/releases/download/v3.26.4/cmake-3.26.4-windows-x86_64.zip --output cmake.zip
      unzip -qq cmake.zip
      mv cmake-3.26.4-windows-x86_64 cmake
    fi
  fi

  if [[ -d "$SDK_DIR/cmake" ]] ; then
    export PATH="$PATH:$SDK_DIR/cmake/bin"
  fi

  if [[ ! -f "$SDK_DIR/llvm/bin/clang.exe" ]] ; then
  (
    SDK_ARCHIVE=sdk-mingw.7z
    curl -L -O "$BASE_SDK/$SDK_ARCHIVE"
    cmake -E tar xzf "sdk-mingw.7z"
    rm "sdk-mingw.7z"
  )
  fi
  ls "$SDK_DIR"

  # Download boost
  if [[ ! -d "$SDK_DIR/boost" ]] ; then
  (
    BOOST="$BOOST_VER.tar.gz"
    curl -L -0 "$BOOST_SDK/$BOOST" --output "$BOOST"
    mkdir -p "$SDK_DIR/boost/include"
    tar xaf "$BOOST"
    mv boost_*/boost "$SDK_DIR/boost/include/"
    rm *.gz
    rm -rf $BOOST
  )
  fi


  # Download ninja
  if [[ ! -f "$SDK_DIR/llvm/bin/ninja.exe" ]] ; then
    if ! command -v ninja >/dev/null 2>&1 ; then
    (
      curl -L -0 https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-win.zip --output ninja-win.zip
      tar xaf ninja-win.zip
      mv ninja.exe "$SDK_DIR/llvm/bin/"
      rm ninja-win.zip
    )
    fi
  fi
)
fi
