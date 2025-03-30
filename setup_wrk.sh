sudo apt-get install build-essential libssl-dev git -y
git clone https://github.com/giltene/wrk2 wrk
cd wrk
make
# move the executable to somewhere in your PATH, ex:
sudo cp wrk /usr/local/bin
