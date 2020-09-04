wget https://github.com/mongodb/mongo-c-driver/releases/download/1.17.0/mongo-c-driver-1.17.0.tar.gz
tar xzf mongo-c-driver-1.17.0.tar.gz
cd mongo-c-driver-1.17.0
mkdir cmake-build
cd cmake-build
cmake -j `nproc` -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
cmake -j `nproc` --build .
sudo cmake --build . --target install
cd ../..
curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.6.0/mongo-cxx-driver-r3.6.0.tar.gz
tar -xzf mongo-cxx-driver-r3.6.0.tar.gz
cd mongo-cxx-driver-r3.6.0/build
cmake -j `nproc` ..                                \
    -DCMAKE_BUILD_TYPE=Release          \
    -DCMAKE_INSTALL_PREFIX=/usr/local
cmake -j `nproc` --build .
sudo cmake --build . --target install
cd ../..
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
