# board_cli Ubuntu Qt 5.12.9 迁移包说明

## 1. 这个目录是做什么的

这个目录是给你单独带到 Ubuntu 虚拟机里的最小命令行版本。

它只保留了板卡端需要的 Qt Console 工程源码，不包含：

- Windows 图形界面 `host_app`
- 早期调试用的 `client`
- 早期调试用的 `server`
- 其他暂时不需要的工程文件

这样做的目的很简单：

1. 减少拷贝内容
2. 降低 Ubuntu 下出错概率
3. 让你更容易定位“板卡端命令行程序”本身的问题

## 2. 这个目录里有什么

- `board_cli.pro`
- `main.cpp`
- `protocol.h`
- `fileutils.h`
- `fileutils.cpp`
- `sender.h`
- `sender.cpp`
- `receiver.h`
- `receiver.cpp`

它本身就是一个完整的 Qt 命令行工程。

## 3. 适配了什么版本

这一版专门兼顾了：

- Windows 上的 Qt 6 + MinGW
- Ubuntu 上的 Qt 5.12.9

其中已经处理了一个常见兼容问题：

- Qt 6 可以写 `Qt::endl`、`Qt::flush`
- Qt 5.12.9 下这类写法容易报错

所以这里已经改成了更稳妥的兼容写法。

## 4. 怎么带到 Ubuntu

你可以把整个 `board_cli_ubuntu_qt5` 文件夹直接复制到 Ubuntu。

例如复制后放到：

```bash
~/FileTransfer/board_cli_ubuntu_qt5
```

## 5. Ubuntu 下如何编译

进入目录后执行：

```bash
cd ~/FileTransfer/board_cli_ubuntu_qt5
qmake board_cli.pro
make -j4
```

如果你的系统里 `qmake` 不是默认命令，也可以先查一下：

```bash
which qmake
qmake -v
```

如果输出里能看到 `Qt version 5.12.9`，说明环境基本对了。

## 6. 编译后会得到什么

成功后会生成可执行文件：

```bash
./board_cli
```

这是一个命令行程序，没有图形界面。

## 7. 如何运行

### 接收模式

```bash
./board_cli --listen --port 8899 --save-dir ./recv
```

作用：

- 监听 8899 端口
- 接收对方发来的文件
- 保存到当前目录下的 `recv` 文件夹

### 发送模式

```bash
./board_cli --send --host 192.168.1.100 --port 8899 --file ./test.bin
```

作用：

- 连接到目标主机 `192.168.1.100`
- 向 8899 端口发送 `test.bin`

说明：

- 现在支持 `~` 和 `~/xxx` 这样的家目录写法
- 例如 `--file ~/test.bin`
- 例如 `--save-dir ~/recv`

### 交互模式

如果你不想每次都手打一大串命令：

```bash
./board_cli
```

或者：

```bash
./board_cli --interactive
```

程序会一步一步问你：

- 发送还是接收
- IP 地址
- 端口
- 文件路径
- 保存目录

## 8. 推荐测试顺序

建议按下面顺序测：

1. 先在 Windows 上验证主机端逻辑
2. 再把这个目录带到 Ubuntu 编译
3. 先做 Ubuntu 和 Windows 的联调
4. 最后再考虑上 i.MX6U 板卡

这样更稳，因为一旦板卡端出问题，你就能判断到底是：

- 代码问题
- Qt 环境问题
- 网络问题
- 板卡系统问题

## 9. 如果编译报错，先看什么

优先检查这几项：

1. `qmake -v` 是否真的是 Qt 5.12.9
2. 当前目录是不是这个工程目录
3. 是否执行了 `qmake board_cli.pro`
4. 是否缺少 `network` 模块

## 10. 这份目录为什么适合交作业

因为它更像一个“板卡端独立子项目”：

- 结构完整
- 依赖清晰
- 目标明确
- 方便老师理解

你在答辩时也更容易说明：

“Windows 端负责图形化操作，板卡端使用独立命令行节点完成文件传输。”
