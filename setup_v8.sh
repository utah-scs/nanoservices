git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:`pwd`/depot_tools
gclient config https://github.com/utah-scs/v8.git
gclient sync
cd v8
sudo apt-get update
./build/install-build-deps.sh
sudo apt-get install -y libhiredis-dev
sudo apt-get install -y python
wget https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-linux.zip
unzip ninja-linux.zip
sudo cp ninja /usr/local/bin/
sudo apt-get install -y clang
sudo apt install generate-ninja
gn gen out/x64.release --args='dcheck_always_on = false is_clang = false is_component_build = false is_debug = false target_cpu = "x64" use_custom_libcxx = false v8_monolithic = true v8_use_external_startup_data = false v8_enable_pointer_compression = false'
ninja -C out/x64.release v8_monolith
cd ../
