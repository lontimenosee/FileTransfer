# board_cli 在 Windows 与 Qt Creator 中的运行说明

## 1. 为什么在 Qt Creator 里输入 `1` 看起来没反应

这是因为 `board_cli` 是一个命令行程序，而 Qt Creator 里的“应用程序输出”窗口并不是一个真正适合交互输入的终端。

它更像是：

- 显示程序输出
- 方便查看日志

但并不适合做“像终端那样一问一答”的交互。

所以当程序显示：

```text
Choose mode [1]:
```

你在“应用程序输出”里输入内容时，程序不一定能像普通终端那样稳定读取。

## 2. 这是不是程序坏了

不是。

从你截图里的输出看，程序其实已经继续运行了，而且默认进入了：

- 接收模式
- 端口 `8899`
- 保存目录 `.`

也就是说，它把空输入当成了默认值，而不是卡死。

## 3. 正确的测试方式是什么

### 方式一：在真正的终端里运行

这是最推荐的方法。

### 方式二：在 Qt Creator 里给程序加完整参数运行

例如：

```text
--listen --port 8899 --save-dir D:/recv
```

或者：

```text
--send --host 127.0.0.1 --port 8899 --file D:/test/demo.txt
```

这样就不依赖交互输入了。

## 4. 如何在 Qt Creator 里填运行参数

1. 打开 `board_cli` 工程
2. 进入左侧 `Projects`
3. 找到 `Run`
4. 在 `Command line arguments` 中填参数

接收示例：

```text
--listen --port 8899 --save-dir D:/Linux/QTcreater/Prj/FileTransfer/test_recv
```

发送示例：

```text
--send --host 127.0.0.1 --port 8899 --file D:/Linux/QTcreater/Prj/FileTransfer/README.md
```

## 5. 为什么你说“我没打包，所以不能直接在终端运行”

这里要区分两件事：

### 终端运行

这不要求你先做正式发布打包。

只要：

- 可执行文件已经编译出来
- Qt 运行库能找到

就可以在终端里直接运行。

### 双击运行或拷给别人运行

这时才更需要部署 Qt 依赖，例如 `windeployqt`。

## 6. 我已经给你补了什么

项目里新增了两个 Windows 脚本：

### 运行脚本

[tools/run_board_cli_windows.ps1](D:\Linux\QTcreater\Prj\FileTransfer\tools\run_board_cli_windows.ps1)

它会：

- 自动找 `board_cli.exe`
- 自动补 Qt / MinGW 的 PATH
- 然后启动程序

你可以直接运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_board_cli_windows.ps1
```

如果要带参数：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_board_cli_windows.ps1 --% --listen --port 8899 --save-dir D:\recv
```

### 部署脚本

[tools/deploy_board_cli_windows.ps1](D:\Linux\QTcreater\Prj\FileTransfer\tools\deploy_board_cli_windows.ps1)

它会对 release 版 `board_cli.exe` 执行 `windeployqt`。

## 7. 你现在最推荐怎么测

### 测试逻辑是否可行

优先这样做：

1. 在 Windows 终端里运行 `board_cli`
2. 不用 Qt Creator 的应用程序输出窗口做交互
3. 或者在 Qt Creator 里直接填写完整参数运行

当前交互模式已经补了两项改进：

1. 在 Windows 终端下优先使用 Unicode 控制台输入，中文路径更稳
2. 端口、文件路径、保存目录输入错误时会提示并允许重新输入，而不是直接卡住

### 测试打包后是否可独立运行

等逻辑测试稳定后，再运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\deploy_board_cli_windows.ps1
```

## 8. 目前最稳的建议

对于 `board_cli`：

1. 交互式模式用真实终端
2. Qt Creator 内优先用“完整参数模式”
3. 逻辑验证通过后，再做部署
