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

参考: https://github.com/kaizouman/gtest-cmake-example
创建一个tests文件夹，里面写有配置gtest相关的CMakeLists.txt
cmake主目录CMakeLists.txt时add_subdirectory到tests文件下继续cmake
```shell
# 本质：执行生产的二进制test文件
$ ./test_main
```
### rpc test demo
examples/test_rpc
### kv test demo
examples/kv
- get, put 操作
### 实现
- 先写测试文件，test-driven的开发，GoogleTest实现
- rpc实现: rpclib库(https://github.com/rpclib/rpclib.git)
  - rpc传输自定type类型，msgpack API来完成：https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_packer#pack-manually
    - 通过在类中定义MSGPACK_DEFINE宏来实现
- part1: Leader选举机制
  - 实现功能
    - raft节点存储的数据内容是什么样的
    - 建立连接(rpc通信网络)，两两可以相互通信
      - 先利用本地多个端口，来实现一个集群
      - 测试的时候要随时模拟机器crash与start
      - 能够异步发送rpc, 并且受到rpc请求能及时处理并回复