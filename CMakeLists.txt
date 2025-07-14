cmake_minimum_required(VERSION 3.10)
project(DigitalRoomSignage)

# Установка стандарта C
set(CMAKE_C_STANDARD 11)

# Поиск SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Директории с исходниками
include_directories(${CMAKE_SOURCE_DIR}/LvglWindowsSimulator)
include_directories(${CMAKE_SOURCE_DIR}/lvgl/src)

# Список исходных файлов
file(GLOB SIMULATOR_SOURCES "${CMAKE_SOURCE_DIR}/LvglWindowsSimulator/*.c")
file(GLOB LVGL_SOURCES 
    "${CMAKE_SOURCE_DIR}/lvgl/src/core/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/display/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/draw/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/font/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/indev/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/layouts/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/misc/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/stdlib/builtin/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/themes/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/tick/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/widgets/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/drivers/sdl/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/drivers/display/*.c"
    "${CMAKE_SOURCE_DIR}/lvgl/src/drivers/indev/*.c"
)

# Исключение Windows-специфичных файлов
list(FILTER SIMULATOR_SOURCES EXCLUDE REGEX ".*lv_win32drv.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*windows/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*lv_windows_.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*osal/lv_windows.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*osal/lv_os.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*fs/win32/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*fs/win32.c$")

# Объединение всех исходных файлов
set(SOURCES ${SIMULATOR_SOURCES} ${LVGL_SOURCES})

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Линковка библиотек
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

# Опции компиляции для LVGL
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)
add_definitions(-DLV_USE_WINDOWS=0)
add_definitions(-DLV_USE_OS=LV_OS_NONE)
add_definitions(-DSDL_MAIN_HANDLED)

# Убедимся, что lv_global.c включён
set_source_files_properties(${CMAKE_SOURCE_DIR}/lvgl/src/core/lv_global.c PROPERTIES COMPILE_FLAGS "-DLV_CONF_INCLUDE_SIMPLE")