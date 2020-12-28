# raft-cpp
C++ implementation of the Raft consensus protocol

### start
Compile third party
```
$ git clone https://github.com/rpclib/rpclib.git
$ cd rpclib
$ cmake .
$ make   
# 可选make install
```
### rpc test demo
examples/test_rpc
### kv test demo
examples/kv
- get, put 操作
### 实现
- rpc实现: rpclib
  - rpc传输自定type类型，msgpack API来完成：https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_packer#pack-manually
    - 通过在类中定义MSGPACK_DEFINE宏来实现