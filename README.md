# FileTransfer

A Qt-based course project for file transfer between a Windows host and an embedded Linux board.

## Overview

This repository contains an embedded systems course project built around a practical file transfer scenario:

- `Windows` acts as the main host with a graphical interface
- `i.MX6U` acts as the headless embedded target
- both sides are designed to support file sending and receiving

The project currently focuses on a stable first milestone:

- TCP-based communication
- single-file transfer
- Windows GUI host application
- board-side command-line program
- beginner-oriented documentation for learning and deployment

## Architecture

The final structure follows the actual hardware constraints of the project.

- `host_app/`
  Windows GUI host based on Qt Widgets, used for daily operation, debugging, and demonstrations
- `board_cli/`
  headless Qt command-line application for Ubuntu and future i.MX6U deployment
- `shared/`
  shared protocol and utility code
- `client/`
  early standalone sender used during the first debugging stage
- `server/`
  early standalone receiver used during the first debugging stage
- `docs/`
  project documentation, deployment notes, debugging notes, and report drafts
- `tools/`
  helper scripts for verification and local testing
- `dist/`
  trimmed transfer package prepared for Ubuntu-side migration

## Repository Layout

```text
FileTransfer/
|-- board_cli/
|-- client/
|-- dist/
|-- docs/
|-- host_app/
|-- server/
|-- shared/
|-- tools/
|-- FileTransfer.pro
`-- README.md
```

## Main Components

### Windows Host

The Windows side provides a unified GUI application for:

- configuring target IP and port
- choosing and sending files
- listening on a local port
- receiving files into a selected directory
- displaying logs and transfer progress

### Board-side CLI

The board-side program is intentionally lightweight and headless:

- `--listen` mode for receiving files
- `--send` mode for actively sending files
- interactive prompt mode for easier manual testing
- Ubuntu-side verification before real board deployment

## Protocol

The project uses a simple custom application-layer protocol on top of TCP:

1. fixed-size header
2. file metadata
3. file content
4. ACK confirmation after successful save

This design avoids treating TCP as if it were message-based and improves transfer reliability across real devices.

## Build

### Windows

Build with the Qt MinGW kit:

- `host_app/host_app.pro`
- `board_cli/board_cli.pro`

The root project file is:

- `FileTransfer.pro`

### Ubuntu

For Ubuntu-side command-line verification, use:

- `dist/board_cli_ubuntu_qt5/`

or build directly from:

- `board_cli/`

## Documentation

All major project documents are indexed here:

- [Documentation Index](./docs/README.md)

Recommended reading order:

1. [项目总览](./docs/01-%E9%A1%B9%E7%9B%AE%E6%80%BB%E8%A7%88.md)
2. [工程目录说明](./docs/02-%E5%B7%A5%E7%A8%8B%E7%9B%AE%E5%BD%95%E8%AF%B4%E6%98%8E.md)
3. [Qt工程搭建说明](./docs/03-Qt%E5%B7%A5%E7%A8%8B%E6%90%AD%E5%BB%BA%E8%AF%B4%E6%98%8E.md)
4. [通信协议设计](./docs/06-%E9%80%9A%E4%BF%A1%E5%8D%8F%E8%AE%AE%E8%AE%BE%E8%AE%A1.md)
5. [统一主机版使用说明](./docs/10-%E7%BB%9F%E4%B8%80%E4%B8%BB%E6%9C%BA%E7%89%88%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.md)
6. [板卡端设计与Ubuntu使用说明](./docs/12-%E6%9D%BF%E5%8D%A1%E7%AB%AF%E8%AE%BE%E8%AE%A1%E4%B8%8EUbuntu%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.md)

## Current Status

Completed work:

- Windows host GUI structure
- board-side CLI structure
- custom TCP file transfer protocol
- progress display and logging
- ACK-based transfer completion confirmation
- Windows local loopback verification
- Ubuntu-side command-line verification
- report draft and deployment notes

## Notes

- Build outputs are intentionally ignored and are not part of the repository source tree.
- Most source documentation is kept in Markdown for easier version tracking.
- `.docx` and `.pptx` files are preserved only when they are directly relevant to course deliverables.
