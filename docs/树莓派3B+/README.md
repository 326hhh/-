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

### 脚本内部实现逻辑

```bash
#!/bin/bash

# 参数说明：
# 第一个参数：工程名（必填）
# 第二个参数：工程存放的父目录（可选，不填则在当前目录创建）

if [ $# -lt 1 ]; then
    echo "用法1（当前目录创建）: ./create_opencv_project.sh 工程名"
    echo "用法2（指定目录创建）: ./create_opencv_project.sh 工程名 目标父目录"
    exit 1
fi

PROJECT_NAME=$1

# 确定父目录：第二个参数有值就用它，没有就用当前目录
if [ -n "$2" ]; then
    PARENT_DIR="$2"
else
    PARENT_DIR=$(pwd)
fi

# 拼接工程完整路径，自动创建不存在的父目录
PROJECT_PATH="${PARENT_DIR}/${PROJECT_NAME}"
mkdir -p "${PROJECT_PATH}"

# 创建标准目录结构
mkdir -p "${PROJECT_PATH}"/{inc,src,build,assets}

# 生成 CMakeLists.txt
cat > "${PROJECT_PATH}/CMakeLists.txt" << EOF_CMAKE
cmake_minimum_required(VERSION 3.10)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV version: \${OpenCV_VERSION}")

include_directories(\${PROJECT_SOURCE_DIR}/inc)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} \${OpenCV_LIBS})
EOF_CMAKE

# 生成 main.cpp 模板
cat > "${PROJECT_PATH}/src/main.cpp" << EOF_CPP
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    return 0;
}
EOF_CPP

echo "✅ 工程创建成功！"
echo "工程完整路径：${PROJECT_PATH}"
echo ""
---

echo "编译运行命令："
echo "  cd ${PROJECT_PATH}/build"
echo "  cmake -DOpenCV_DIR=/usr/local/lib/cmake/opencv4 .."
echo "  make -j2"
echo "  ./${PROJECT_NAME}"
```

## 编译opencv工程
