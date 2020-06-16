# my_ls
## 背景

该程序使用C语言在Linux环境中实现了GNU/Linux的命令ls的部分功能，取名为my_ls。

## 安装

本程序无需安装

## 使用

使用之前需要安装一个curses库

```bash
sudo apt-get install libncurses5-dev
```

之后在编译C语言源文件

```bash
$ gcc my_ls.c -o my_ls -lncurses -lm
$ ./my_ls
README.md my_ls my_ls.c tempCodeRunnerFile.cpp test test.c
```
## 主要项目负责人

xtian