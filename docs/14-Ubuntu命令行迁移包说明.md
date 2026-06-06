# Ubuntu 命令行迁移包说明

## 1. 这次做了什么

为了减少你把项目转到 Ubuntu 时的出错概率，这次我专门整理了一份“最小可转移版本”。

目录位置：

- [dist/board_cli_ubuntu_qt5/README_ubuntu.md](D:/Linux/QTcreater/Prj/FileTransfer/dist/board_cli_ubuntu_qt5/README_ubuntu.md)

对应源码目录：

- [board_cli/main.cpp](D:/Linux/QTcreater/Prj/FileTransfer/board_cli/main.cpp)
- [board_cli/sender.cpp](D:/Linux/QTcreater/Prj/FileTransfer/board_cli/sender.cpp)
- [board_cli/receiver.cpp](D:/Linux/QTcreater/Prj/FileTransfer/board_cli/receiver.cpp)
- [board_cli/board_cli.pro](D:/Linux/QTcreater/Prj/FileTransfer/board_cli/board_cli.pro)

## 2. 为什么要额外做一个迁移包

因为你当前最需要的是：

1. 先把命令行版本稳定带到 Ubuntu
2. 先验证 Ubuntu / 板卡方向的收发逻辑
3. 避免把 Windows 图形界面部分一起带过去造成额外错误

如果直接把整个工程搬过去，常见问题会更多：

- Qt 套件不匹配
- GUI 模块依赖太多
- 构建目录混在一起
- 不知道到底是哪一部分出错

所以这次采取“拆小、拆清楚”的办法。

## 3. 迁移包里保留了什么

迁移包只保留 `board_cli` 独立工程需要的文件：

- `board_cli.pro`
- `main.cpp`
- `protocol.h`
- `fileutils.h`
- `fileutils.cpp`
- `sender.h`
- `sender.cpp`
- `receiver.h`
- `receiver.cpp`
- `README_ubuntu.md`
- `build.sh`

这样你把它单独复制到 Ubuntu 后，它本身就是一个完整的 Qt Console 工程。

## 4. 这次顺手做了什么兼容处理

为了适配你 Ubuntu 上大概率使用的 `Qt 5.12.9`，这次把命令行端做了兼容整理。

重点是下面这个问题：

- 在较新的 Qt 里，有些代码会写 `Qt::endl`、`Qt::flush`
- 但在 Qt 5.12.9 里，这类写法容易直接报编译错误

所以现在命令行端改成了更稳妥的写法：

1. 用 `"\n"` 输出换行
2. 需要立刻显示提示符时，手动调用 `flush()`

这个改动的目的不是“写法更高级”，而是“让 Qt 5 和 Qt 6 都更容易通过”。

## 5. 你实际要带走哪个目录

建议你直接带走这个目录：

- [dist/board_cli_ubuntu_qt5](D:/Linux/QTcreater/Prj/FileTransfer/dist/board_cli_ubuntu_qt5)

这是最省事的办法。

## 6. 带到 Ubuntu 后怎么做

### 第一步：进入目录

```bash
cd ~/FileTransfer/board_cli_ubuntu_qt5
```

### 第二步：确认 Qt 版本

```bash
qmake -v
```

你重点看输出里是否有类似：

```bash
Qt version 5.12.9
```

### 第三步：编译

你可以手动编译：

```bash
qmake board_cli.pro
make -j4
```

也可以直接执行脚本：

```bash
sh build.sh
```

## 7. 编译成功后怎么运行

### 接收

```bash
./board_cli --listen --port 8899 --save-dir ./recv
```

### 发送

```bash
./board_cli --send --host 192.168.1.100 --port 8899 --file ./test.bin
```

### 交互运行

```bash
./board_cli
```

## 8. 为什么这比直接在 Qt Creator 里点运行更适合你

因为你后面真正上板时，板卡本身就是无界面的。

也就是说，最终使用方式更接近：

```bash
./board_cli --listen ...
```

而不是 Qt Creator 里的“绿色三角按钮”。

现在先在 Ubuntu 命令行里跑通，后面移植到板卡时，步骤和思路会更一致。

## 9. 这次工作对你当前目标的意义

你现在的目标不是把整套系统一次性全部跑通，而是先降低迁移难度。

这份迁移包的价值就在于：

1. 缩小问题范围
2. 减少不必要依赖
3. 更接近板卡最终运行形态
4. 更适合写进课设报告

## 10. 你接下来最建议做什么

建议按这个顺序：

1. 先把 `dist/board_cli_ubuntu_qt5` 复制到 Ubuntu
2. 在 Ubuntu 编译 `board_cli`
3. 用 Ubuntu `board_cli` 和 Windows `host_app` 联调
4. 联调稳定后，再开始整理真正的 i.MX6U 上板步骤
