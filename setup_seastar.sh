sudo apt-get update
git submodule update --init --recursive
sudo apt-get install -y build-essential linux-image-extra-virtual
cd seastar
sudo ./install-dependencies.sh
sudo apt-get install -y libunistring-dev
sudo apt-get install -y libudev-dev
./configure.py --cook fmt
./configure.py --mode=release
ninja -C build/release
cd ..
echo "fs.aio-max-nr = 1048576" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p /etc/sysctl.conf
