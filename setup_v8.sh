git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:`pwd`/depot_tools
gclient config https://github.com/utah-scs/v8.git
gclient sync
cd v8
sudo apt-get update
./build/install-build-deps.sh
sudo apt-get install -y libhiredis-dev
sudo apt-get install -y python
git clone https://gn.googlesource.com/gn
wget https://github.com/ninja-build/ninja/releases/download/v1.9.0/ninja-linux.zip
unzip ninja-linux.zip
sudo cp ninja /usr/local/bin/
sudo apt-get install -y clang
cd gn
git reset --hard 6c484875cdcbfeba43a4fae0106a1be8ca65eff1
python build/gen.py
sudo ninja -C out
sudo cp out/gn /usr/local/bin/
cd ../
gn gen x64.release --args='is_debug=false target_cpu="x64" v8_monolithic = true v8_use_external_startup_data = false use_custom_libcxx = false is_component_build = false'
ninja -C x64.release v8_monolith
cd ../
