# GitHub 仓库整理说明

## 1. 为什么课程设计也要按 GitHub 项目标准整理

很多同学会把课程设计只当作“能跑就行”的作业，但如果目录规范、说明清楚，老师会更容易看出你的工程意识。

一个像样的 GitHub 项目，至少应该有：

- 清晰的目录结构
- `README.md`
- `.gitignore`
- 设计说明文档

这不仅是为了“好看”，更是为了让别人能快速理解你的项目。

## 2. 当前项目已经具备的基础元素

本项目当前已经包含：

- 根工程 `FileTransfer.pro`
- 客户端与服务端子工程
- 公共代码目录 `shared/`
- 说明文档目录 `docs/`
- `README.md`
- `.gitignore`

这已经达到了一个比较标准的课程项目仓库起点。

## 3. 你后面上传到 GitHub 的基本步骤

在项目根目录执行：

```bash
git add .
git commit -m "Initial Qt file transfer project"
```

然后在 GitHub 上创建一个空仓库，再执行：

```bash
git remote add origin 你的仓库地址
git branch -M main
git push -u origin main
```

## 4. 后续建议补充的仓库内容

为了让项目更完整，你后面可以继续加入：

- 运行截图
- 板卡部署记录
- 交叉编译记录
- 报告 PDF
- 答辩 PPT

## 5. 提交时注意什么

不要把下面这些内容上传到仓库：

- `build` 目录
- Qt Creator 生成的 `.pro.user`
- 大量临时日志
- 无关软件安装包

这也是 `.gitignore` 存在的原因。

## 6. 一个适合课程设计的提交习惯

建议你以后按阶段提交，例如：

- `build: create qmake project skeleton`
- `feat: add tcp file sender client`
- `feat: add tcp file receiver server`
- `docs: add beginner setup notes`

这样老师如果看提交历史，也会发现你的开发过程是有条理的。
