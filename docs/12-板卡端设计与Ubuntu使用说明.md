# 板卡端设计与 Ubuntu 使用说明

## 1. 为什么要单独做板卡端工程

你的 `i.MX6U` 没有图形界面，所以板卡端不适合继续使用 `Qt Widgets`。

因此本项目专门新增了一个独立工程：

- [board_cli/board_cli.pro](D:\Linux\QTcreater\Prj\FileTransfer\board_cli\board_cli.pro)

这个工程的定位是：

- 面向无界面嵌入式板卡
- 采用命令行方式运行
- 独立于 Windows 图形主机
- 可以直接打包到 Ubuntu 虚拟机里编译

## 2. 为什么这次不复用 shared

这次按你的要求，`board_cli` 不复用现有 `shared/`。

这样做的好处是：

1. 板卡端工程更完整独立
2. 你拷到 Ubuntu 后可以单独打开、单独编译
3. 更符合“板端完整实现”的课程设计表达

## 3. board_cli 能做什么

`board_cli` 支持两种模式：

### 接收模式

板卡作为接收端，监听端口并保存文件。

示例：

```bash
./board_cli --listen --port 8899 --save-dir /home/ubuntu/recv
```

### 发送模式

板卡作为发送端，把一个文件发给 Windows 主机。

示例：

```bash
./board_cli --send --host 192.168.1.100 --port 8899 --file /home/ubuntu/test.bin
```

另外，如果你不想每次输入一长串命令，现在也可以直接运行：

```bash
./board_cli
```

程序会进入交互式模式，分步骤询问：

- 发送还是接收
- IP
- 端口
- 文件路径
- 保存目录

这更适合先在 Windows 或 Ubuntu 上测试逻辑。

## 4. 目录结构

`board_cli/` 目录中包含：

- `main.cpp`
- `protocol.h`
- `fileutils.h`
- `fileutils.cpp`
- `sender.h`
- `sender.cpp`
- `receiver.h`
- `receiver.cpp`
- `board_cli.pro`

这是一个完整独立的小型 Qt Console 工程。

## 5. 如何打包到 Ubuntu

你可以直接把整个项目目录打包过去，也可以只带下面这些目录：

- `board_cli/`
- `docs/`

如果你想最省事，建议直接把整个 `FileTransfer` 项目目录打包复制到 Ubuntu。

## 6. 在 Ubuntu 上如何编译

进入 Ubuntu 虚拟机后：

```bash
cd FileTransfer/board_cli
qmake board_cli.pro
make -j4
```

如果系统里 `qmake` 不在 PATH，需要先确认 Qt 环境。

当前这份 `board_cli` 已按 `Qt 5.12.9` 和 `Qt 6` 的共同兼容写法整理，尤其处理了网络错误信号在不同 Qt 版本下的差异，所以更适合你现在的 Ubuntu 环境。

## 7. 编译成功后会得到什么

成功后会生成：

- `board_cli`

这是一个命令行程序，没有图形界面。

## 8. 如何与 Windows 主机配合

### 情况一：Windows 发，板卡收

1. 板卡运行：

```bash
./board_cli --listen --port 8899 --save-dir /home/ubuntu/recv
```

2. Windows 端 `host_app`：

- 连接板卡 IP
- 端口填 `8899`
- 选择文件并发送

### 情况二：板卡发，Windows 收

1. Windows 端 `host_app` 先开始监听
2. 板卡运行：

```bash
./board_cli --send --host 192.168.1.100 --port 8899 --file /home/ubuntu/test.bin
```

## 9. 程序输出怎么看

`board_cli` 会在终端输出日志，例如：

- 是否连接成功
- 是否开始发送
- 是否开始接收
- 当前进度
- 最终保存路径
- 是否收到 ACK

这些日志对调试非常重要。

## 9.1 两种使用方式怎么选

### 方式一：完整命令行参数

适合：

- 脚本化测试
- 报告中展示标准命令
- 以后做自动化

### 方式二：交互式模式

适合：

- 人手动调试
- 临时改 IP、端口、文件路径
- 在 Windows 或 Ubuntu 终端里快速验证逻辑

你可以直接运行：

```bash
./board_cli
```

或者：

```bash
./board_cli --interactive
```

## 10. 交叉编译和 Ubuntu 编译的关系

你现在提到的是：

- 有 Ubuntu 虚拟机
- 有 Qt 环境

这意味着你可以先做“Ubuntu 环境下编译和验证”。

这一步的价值是：

1. 先证明板卡端命令行工程是成立的
2. 先把代码逻辑跑通
3. 后面再继续做真正 ARM 交叉编译时，难度会小很多

也就是说，建议开发顺序是：

1. 先在 Windows 或 Ubuntu 上编译 `board_cli`
2. 先在 PC 环境验证发送/接收逻辑
3. 再在 Ubuntu 或板端环境做网络联调
4. 最后补交叉编译说明和实际上板过程

## 10.1 为什么建议先在 Windows 或 Ubuntu 上测试逻辑

因为板卡调试时，问题来源很多：

- 网络是否正常
- 路径权限是否正常
- 文件系统是否可写
- Qt 环境是否完整

如果先在 PC 环境把 `board_cli` 的逻辑跑通，就能把问题范围缩小很多。

推荐顺序是：

1. `Windows host_app` 与 `Windows board_cli.exe` 联调
2. Ubuntu 中编译并运行 `board_cli`
3. Ubuntu `board_cli` 与 Windows `host_app` 联调
4. 最后迁移到板卡

## 11. 课设里怎么描述这部分

你在报告里可以这样写：

> 考虑到 i.MX6U 开发板不具备图形界面，系统设计中新增了独立的命令行文件传输节点 board_cli。该节点基于 QtCore 和 QtNetwork 实现，支持监听接收模式与主动发送模式。开发阶段可先在 Ubuntu 环境中单独编译与验证，再进一步部署到嵌入式 ARM 平台。

## 12. 后续我还可以继续帮你补什么

后面我还可以继续补：

1. `docs/13-交叉编译准备说明.md`
2. `board_cli` 的实际测试命令模板
3. Windows 主机与板卡端的联调流程文档
