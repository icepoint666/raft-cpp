# raft-cpp
C++ implementation of the Raft consensus protocol

### start
Compile third party
**rpclib**
```shell
$ git clone https://github.com/rpclib/rpclib.git
$ cd rpclib
$ cmake .
$ make   
# 可选make install
```
**gtest**
创建一个tests文件夹，里面写有配置GTest相关的CMakeLists.txt
cmake主目录CMakeLists.txt时add_subdirectory到tests文件下继续cmake
```shell
# 本质：执行生产的二进制test文件
$ ./test_main
```
```
### rpc test demo
examples/test_rpc
### kv test demo
examples/kv
- get, put 操作
### 实现
- 先写测试文件，test-driven的开发，Gtest实现
- rpc实现: rpclib库(https://github.com/rpclib/rpclib.git)
  - rpc传输自定type类型，msgpack API来完成：https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_packer#pack-manually
    - 通过在类中定义MSGPACK_DEFINE宏来实现
- part1: Leader选举机制
  - 实现功能
    - 建立连接，建立一个rpc通信网络，两两相连，测试的时候要随时模拟机器crash与start