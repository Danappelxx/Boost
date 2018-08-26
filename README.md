# Boost
Expose 2011 SAAB 9-3 CAN interfaces over Bluetooth Low Energy.

## Build Notes

### Typical build

```shell
particle compile duo --saveTo firmware.bin
particle flash <device> firmware.bin
```

### Offline build

Compiling with particle requires internet, as it actually compiles over the air. To do it locally and offline is a bunch of work. Important commands:

#### Download firmware

```shell
git clone https://github.com/particle-iot/firmware.git --branch feature/new-platform-duo /usr/local/particle-firmware
```

#### Install GCC ARM Embedded and dfu-util

```shell
brew tap PX4/homebrew-px4
brew update
echo -e "
require 'formula'

class GccArmNoneEabi53 < Formula
  homepage 'https://launchpad.net/gcc-arm-embdded'
  version '20160307'
  url 'https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q1-update/+download/gcc-arm-none-eabi-5_3-2016q1-20160330-mac.tar.bz2'
  sha256 '480843ca1ce2d3602307f760872580e999acea0e34ec3d6f8b6ad02d3db409cf'

  def install
    ohai 'Copying binaries...'
    system 'cp', '-rv', 'arm-none-eabi', 'bin', 'lib', 'share', \"#{prefix}/\"
  end
end" > /usr/local/Homebrew/Library/Taps/px4/homebrew-px4/gcc-arm-none-eabi-53.rb
brew install px4/px4/gcc-arm-none-eabi-53 dfu-util
```

#### Download library locally

```shell
particle library copy carloop
```

#### Command Line
In any directory, run:

```shell
make --directory=/usr/local/particle-firmware/modules all PLATFORM=duo APPDIR=$PROJECT_DIR
```

Firmware outputs in `$PROJECT_DIR/target/Boost.bin`.

#### Xcode
Build the `firmware` target in Xcode, firmware outputs in `$PROJECT_DIR/target/Vortex.bin`!
