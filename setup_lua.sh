git clone https://luajit.org/git/luajit.git
cd luajit
make && sudo make install
cd ..
wget https://luarocks.org/releases/luarocks-3.11.1.tar.gz
tar zxpf luarocks-3.11.1.tar.gz
cd luarocks-3.11.1
./configure && make && sudo make install
sudo luarocks install luasocket
cd ..
