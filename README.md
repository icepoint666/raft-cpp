# raft-cpp
C++ implementation of the Raft consensus protocol

### start
Compile third party

**mushroom**
包含rpc服务的API

**gtest**

参考: https://github.com/kaizouman/gtest-cmake-example
创建一个test文件夹，里面写有配置gtest相关的CMakeLists.txt

编译
```shell
$ bash run.sh test #大的修改执行脚本，小的修改直接在build里make
```
测试
```shell
$ ./testcase --gtest_filter=RPC.server # 启动服务器
$ ./testcase --gtest_filter=RPC.client # 测试rpc连接
Note: Google Test filter = RPC.Client
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from RPC
[ RUN      ] RPC.Client
total  : 3
success: 3
failure: 0
bad    : 0
[       OK ] RPC.Client (1 ms)
[----------] 1 test from RPC (1 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (1 ms total)
[  PASSED  ] 1 test.

```
