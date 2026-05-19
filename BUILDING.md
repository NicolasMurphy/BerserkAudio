# Building BerserkAudio

## VCV Rack

See https://vcvrack.com/manual/Building

```bash
export RACK_DIR=~/Rack-SDK
make install
```

### Installing a .vcvplugin from CI

If you need to test a `.vcvplugin` artifact from GitHub Actions/Release (rather than building locally), the file is zstd-compressed and needs to be manually extracted:

```bash
brew install zstd  # if not already installed
zstd -d ~/Downloads/BerserkAudio-2.0.0-mac-arm64.vcvplugin -o ~/Downloads/BerserkAudio-2.0.0-mac-arm64.tar
tar -xf ~/Downloads/BerserkAudio-2.0.0-mac-arm64.tar -C '/Users/nicolasmurphy/Library/Application Support/Rack2/plugins-mac-arm64/'
```

Then restart Rack. The plugins directory may differ by platform and architecture.

## MetaModule

> **Note:** The paths below assume a `~/projects/` layout (e.g. `~/projects/BerserkAudio`, `~/projects/metamodule-plugin-sdk`). Adjust to match your machine.

### Prerequisites (one-time setup)

1. **ARM GCC Toolchain v12.2 or v12.3** (must be this version — newer won't work)

   Download `arm-gnu-toolchain-12.3.rel1-darwin-arm64-arm-none-eabi.tar.xz` from the
   [Arm Developer site](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
   and extract it:

   ```bash
   mkdir -p ~/toolchains
   tar -xf ~/Downloads/arm-gnu-toolchain-12.3.rel1-darwin-arm64-arm-none-eabi.tar.xz -C ~/toolchains
   ```

2. **CMake** (v3.22+) and **Ninja**

   ```bash
   brew install cmake ninja
   ```

3. **MetaModule Plugin SDK**

   ```bash
   git clone https://github.com/4ms/metamodule-plugin-sdk --recursive ~/projects/metamodule-plugin-sdk
   ```

### Configure (first time, or after changing CMakeLists.txt)

This generates the `mm-build/` directory. You only need to re-run this when `CMakeLists.txt` changes.

```bash
cd ~/projects/BerserkAudio
cmake --fresh -B mm-build -G Ninja \
  -DMETAMODULE_SDK_DIR=$HOME/projects/metamodule-plugin-sdk \
  -DTOOLCHAIN_BASE_DIR=$HOME/toolchains/arm-gnu-toolchain-12.3.rel1-darwin-arm64-arm-none-eabi/bin
```

### Build

```bash
cmake --build mm-build
```

The output file lands in `metamodule-plugins/BerserkAudio.mmplugin`.

Rename it to include the version when testing:

```bash
mv metamodule-plugins/BerserkAudio.mmplugin metamodule-plugins/BerserkAudio-v2.0.0.mmplugin
```

## Releasing

To release a new version (both VCV Rack and MetaModule):

```bash
./release.sh 2.1.0
```

This will:

1. Update the version in `plugin.json` and `CMakeLists.txt`
2. Commit the change
3. Push a `v2.1.0` tag, which triggers the build workflow
4. The workflow builds VCV Rack plugins for win-x64, lin-x64, mac-x64, mac-arm64, builds the MetaModule plugin, and creates a single GitHub release with all artifacts attached

Requires `jq` to be installed (`brew install jq`).

### Panel artwork

The MetaModule cannot render SVGs. The panel PNGs in `assets/` were generated from the panel SVGs in `res/` using the SDK's conversion script:

```bash
export INKSCAPE_BIN_PATH=/Applications/Inkscape.app/Contents/MacOS/inkscape
cd ~/projects/metamodule-plugin-sdk
python3 scripts/SvgToPng.py --input ~/projects/BerserkAudio/res --output ~/projects/BerserkAudio/assets --height=240
```

Re-run this if you change any panel SVG. The PNGs must be 240 pixels tall.
