cmake_minimum_required(VERSION 3.10)
project(DigitalRoomSignage)

# Установка стандарта C
set(CMAKE_C_STANDARD 11)

# Поиск SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Добавление директорий с исходниками
include_directories(${CMAKE_SOURCE_DIR}/LvglWindowsSimulator)
include_directories(${CMAKE_SOURCE_DIR}/lvgl/src)
include_directories(${CMAKE_SOURCE_DIR}/lv_drivers)

# Список исходных файлов
file(GLOB_RECURSIVE SOURCES
    ${CMAKE_SOURCE_DIR}/LvglWindowsSimulator/*.c
    ${CMAKE_SOURCE_DIR}/lvgl/src/*.c
    ${CMAKE_SOURCE_DIR}/lv_drivers/*.c
)

# Исключение Windows-специфичных файлов
list(FILTER SOURCES EXCLUDE REGEX ".*lv_win32drv.c$")

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Линковка библиотек
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

# Опции компиляции для LVGL
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)