mkdir -p third_party

if [ ! -f "./third_party/eigen.tar.gz" ]; then
wget https://gitlab.com/libeigen/eigen/-/archive/3.3.9/eigen-3.3.9.tar.gz -O ./third_party/eigen.tar.gz
cd third_party
tar zxvf eigen.tar.gz 
mv eigen-3.3.9 eigen3
cd ..
fi
if [ ! -f "./third_party/abseil.tar.gz" ]; then
wget https://github.com/abseil/abseil-cpp/archive/20200923.3.tar.gz -O ./third_party/abseil.tar.gz
cd third_party
tar zxvf abseil.tar.gz
mv abseil-cpp-20200923.3 abseil-cpp
cd ..
fi
# if [ ! -f "./third_party/opencv.zip" ]; then 
# wget https://github.com/opencv/opencv/archive/4.5.1.zip -O ./third_party/opencv.zip 
# cd third_party 
# unzip opencv.zip 
# mv opencv-4.5.1 opencv 
# cd opencv 
# mkdir -p build && cd build 
# cmake -GNinja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../libs -DCMAKE_CXX_COMPILER=/usr/bin/g++-5 -DCMAKE_C_COMPILER=/usr/bin/gcc-5
# cmake --build . -j20 
# cd ..
# fi 

# mkdir -p build && cd build 
# cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/g++-5 -DCMAKE_C_COMPILER=/usr/bin/gcc-5 ..
# cmake --build . -j20
