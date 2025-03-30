sudo apt-get update
sudo apt-get install build-essential libssl-dev git -y
sudo apt-get install zlib1g-dev 
git clone https://github.com/giltene/wrk2 wrk
cd wrk
make
# move the executable to somewhere in your PATH, ex:
sudo cp wrk /usr/local/bin
