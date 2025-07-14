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
include_directories(${CMAKE_SOURCE_DIR}/lv_drivers)

# Список исходных файлов
file(GLOB SIMULATOR_SOURCES "${CMAKE_SOURCE_DIR}/LvglWindowsSimulator/*.c")
file(GLOB LVGL_SOURCES "${CMAKE_SOURCE_DIR}/lvgl/src/*.c" "${CMAKE_SOURCE_DIR}/lvgl/src/*/*.c")
file(GLOB LV_DRIVERS_SOURCES 
    "${CMAKE_SOURCE_DIR}/lv_drivers/sdl/*.c"
    "${CMAKE_SOURCE_DIR}/lv_drivers/display/*.c"
    "${CMAKE_SOURCE_DIR}/lv_drivers/indev/*.c"
)

# Исключение Windows-специфичных файлов
list(FILTER SIMULATOR_SOURCES EXCLUDE REGEX ".*lv_win32drv.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*windows/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*lv_windows_.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*osal/.*") # Исключение osal
list(FILTER LV_DRIVERS_SOURCES EXCLUDE REGEX ".*windows/.*")
list(FILTER LV_DRIVERS_SOURCES EXCLUDE REGEX ".*lv_win32drv.c$")
list(FILTER LV_DRIVERS_SOURCES EXCLUDE REGEX ".*lv_windows_.*")

# Объединение всех исходных файлов
set(SOURCES ${SIMULATOR_SOURCES} ${LVGL_SOURCES} ${LV_DRIVERS_SOURCES})

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Линковка библиотек
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

# Опции компиляции для LVGL
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)
add_definitions(-DLV_USE_WINDOWS=0)
add_definitions(-DSDL_MAIN_HANDLED)