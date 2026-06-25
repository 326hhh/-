# 树莓派使用指南

## 创建自动opencv工程脚本

为了更好在树莓派上开发opencv，我使用豆包创建了create_opencv_project.sh自动化配置脚本，脚本位于根目录下。

### 使用教程

创建一个新工程

#### 命令格式:

~/create_opencv_project.sh <工程名> <目标路径>

#### 示例
在 ~/pi/projects 目录下创建一个名为 test_demo 的工程:

~/create_opencv_project.sh test_demo ~/pi/projects
