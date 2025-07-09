cmake_minimum_required(VERSION 3.11)
project(Digital-Room-Signage)
add_subdirectory(LvglPlatform/lvgl)
# Это чисто пофиксить пайплайн, мб оптимальнее сделать можно https://github.com/Fe1qqu/Digital-Room-Signage/actions/runs/16180766316
# Перелопатил директории, чтоб билд не падал
include_directories(
    ${CMAKE_SOURCE_DIR}/LvglPlatform/lvgl
    ${CMAKE_SOURCE_DIR}/LvglPlatform/LvglWindowsIconResource
    ${CMAKE_SOURCE_DIR}/LvglPlatform
)
add_executable(my_app 
            LvglWindowsSimulator/main.c
            LvglWindowsSimulator/schedule_data.c
            LvglWindowsSimulator/schedule_ui.c
            LvglWindowsSimulator/time_date_display.c
            )

# Связываем LVGL с вашим проектом
target_include_directories(my_app PRIVATE
    ${CMAKE_SOURCE_DIR}/LvglPlatform/lvgl
)