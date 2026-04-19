# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS components/Middlewares)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(face_ai_project)

# 将 spiffs_image/ 目录打包成 SPIFFS 镜像，烧录到 storage 分区
spiffs_create_partition_image(storage spiffs_image FLASH_IN_PROJECT)
