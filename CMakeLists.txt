cmake_minimum_required(VERSION 3.11)
project(Digital-Room-Signage)
add_subdirectory(LvglPlatform/lvgl)

add_executable(my_app 
            LvglWindowsSimulator/main.c
            LvglWindowsSimulator/schedule_data.c
            LvglWindowsSimulator/schedule_ui.c
            LvglWindowsSimulator/time_date_display.c
            )

# Связываем LVGL с вашим проектом
target_link_libraries(my_app PRIVATE lvgl)