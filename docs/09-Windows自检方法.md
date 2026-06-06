# Windows 自检方法

## 1. 为什么要做自检

在真正把程序搬到 `i.MX6U` 之前，最稳妥的办法不是直接上板，而是先在 Windows 上做一次完整自检。

这样做的目的有两个：

1. 先确认协议和文件传输逻辑是正确的
2. 把问题尽量提前暴露，减少板端调试难度

## 2. 自检的核心思路

自检时，不需要开发板参与。

我们在同一台 Windows 电脑上运行自检。

推荐有两种方法：

- 方法一：使用统一主机版 `host_app/`
- 方法二：继续使用旧的 `client/ + server/` 调试版

然后使用回环地址：

`127.0.0.1`

让发送端把文件发给本机上的接收端。

如果：

- 能成功连接
- 能完整收到文件
- 接收后的文件和原文件哈希值一致

就说明当前文件传输基础逻辑是可行的。

## 3. 自检流程

### 推荐方案：使用统一主机版

这是当前更推荐的方法，因为它更符合你后面“Windows 作为主机”的目标。

1. 运行 `FileTransferHost`
2. 在接收区点击 `Start Listen`
3. 在发送区连接 `127.0.0.1:8899`
4. 选择测试文件
5. 点击 `Send File`

关于统一主机版的详细说明，请查看：

[docs/10-统一主机版使用说明.md](D:\Linux\QTcreater\Prj\FileTransfer\docs\10-统一主机版使用说明.md)

### 备用方案：使用旧版 sender/receiver

如果你只是想先验证旧工程，也可以继续按下面步骤操作。

### 第一步：准备一个测试文件

建议先准备一个容易识别的小文件，例如：

- 一个 `.txt`
- 一个 `.png`
- 一个 `.pdf`

不要一开始就用很大的文件。

### 第二步：启动接收端

运行接收端程序后：

1. 保持端口为 `8899`
2. 选择一个专门的保存目录，例如：
   `D:\Linux\QTcreater\Prj\FileTransfer\selfcheck_recv`
3. 点击开始监听

### 第三步：启动发送端

运行发送端程序后：

1. 服务器 IP 填写 `127.0.0.1`
2. 端口填写 `8899`
3. 点击连接
4. 选择测试文件
5. 点击发送

### 第四步：观察结果

正常情况下你会看到：

- 发送端显示连接成功
- 发送端进度条到 `100%`
- 接收端显示收到文件
- 接收端进度条到 `100%`
- 保存目录里出现接收到的文件

## 4. 如何确认“不是看起来成功，而是真的成功”

很多初学者会误以为：

“文件传过去了，目录里也有了，就算成功。”

但更严谨的做法是比对文件哈希值。

哈希值一致，才说明接收后的文件内容和原文件完全一致。

## 5. 本项目附带的哈希校验脚本

项目中已经提供了一个 Windows PowerShell 脚本：

[tools/compare_file_hash.ps1](D:\Linux\QTcreater\Prj\FileTransfer\tools\compare_file_hash.ps1)

它会对两个文件计算 `SHA256`，并告诉你是否完全一致。

## 6. 使用脚本的方法

在 PowerShell 中进入项目目录后，执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\compare_file_hash.ps1 `
  -SourceFile "原始文件路径" `
  -ReceivedFile "接收后的文件路径"
```

例如：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\compare_file_hash.ps1 `
  -SourceFile "D:\test\demo.txt" `
  -ReceivedFile "D:\Linux\QTcreater\Prj\FileTransfer\selfcheck_recv\demo.txt"
```

## 7. 看到什么结果才算通过

如果脚本输出：

```text
Result   : PASS - files are identical
```

就表示：

- 文件传输完成
- 接收内容和原始内容完全一致
- 当前基础方案可行

## 8. 自检通过后意味着什么

Windows 本机自检通过后，可以说明：

1. TCP 通信机制基本正确
2. 应用层协议解析基本正确
3. 文件保存逻辑基本正确
4. 后续上板时，主要只剩下网络连通和平台适配问题

也就是说，自检通过并不代表板端已经完全没问题，但它能帮你把最核心的逻辑先确认下来。

## 9. 自检通过后下一步做什么

推荐下一步是：

1. 让 Windows 端逐步具备“发送+接收”双能力
2. 编写 `i.MX6U` 命令行版本
3. 在板端和 Windows 主机之间做真实联网测试
