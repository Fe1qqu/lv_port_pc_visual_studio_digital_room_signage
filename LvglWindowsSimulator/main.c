#include <unistd.h>
#include <SDL2/SDL.h>
//#include "drivers/sdl/lv_sdl_mouse.h"
//#include "drivers/sdl/lv_sdl_mousewheel.h"
//#include "drivers/sdl/lv_sdl_keyboard.h"
#include <time.h>
#include "schedule_ui.h"
#include "time_date_display.h"

#include <../lvgl/lvgl.h>
#include "../lvgl/src/drivers/lv_drivers.h"
#include <../lvgl/src/drivers/sdl/sdl.h>

static lv_display_t* lvDisplay;
static lv_indev_t* lvMouse;
static lv_indev_t* lvMouseWheel;
static lv_indev_t* lvKeyboard;
static lv_timer_t* minute_timer;

#if LV_USE_LOG != 0
static void lv_log_print_g_cb(lv_log_level_t level, const char* buf)
{
    LV_UNUSED(level);
    LV_UNUSED(buf);
}
#endif

static void minute_tick(lv_timer_t* timer)
{
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    if (t->tm_sec == 0) // Trigger only when seconds are 0
    {
        update_time_and_date_display();
        update_progress_bar();
    }
}

int main()
{
    /* initialize lvgl */
    lv_init();

    // Workaround for sdl2 `-m32` crash
    // https://bugs.launchpad.net/ubuntu/+source/libsdl2/+bug/1775067/comments/7
#ifndef WIN32
    setenv("DBUS_FATAL_WARNINGS", "0", 1);
#endif

    /* Register the log print callback */
#if LV_USE_LOG != 0
    lv_log_register_print_cb(lv_log_print_g_cb);
#endif

    /* Add a display
    * Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/

    lvDisplay = lv_sdl_window_create(800, 480);
    lvMouse = lv_sdl_mouse_create();
    lvMouseWheel = lv_sdl_mousewheel_create();
    lvKeyboard = lv_sdl_keyboard_create();

    // Инициализация UI компонентов
    init_time_and_date_display();
    init_schedule_ui();

    // Создание таймера для обновления времени
    minute_timer = lv_timer_create(minute_tick, 1000, NULL);

    Uint32 lastTick = SDL_GetTicks();
    while (1) {
        SDL_Delay(5);
        Uint32 current = SDL_GetTicks();
        lv_tick_inc(current - lastTick); // Update the tick timer. Tick is new for LVGL 9
        lastTick = current;
        lv_timer_handler(); // Update the UI-
    }

    return 0;
}
