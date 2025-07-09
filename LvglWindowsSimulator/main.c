#include <LvglWindowsIconResource.h>
#include <lvgl/lvgl.h>
#include "schedule_ui.h"
#include "time_date_display.h"
//#include "events.h"
#include <time.h>

static lv_timer_t* minute_timer;

static void minute_tick(lv_timer_t* timer)
{
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    if (t->tm_sec == 0) // Trigger only when seconds are 0
    {
        update_time_date_display();
        update_schedule_display();
    }
}

int main()
{
    lv_init();

    /*
     * Optional workaround for users who wants UTF-8 console output.
     * If you don't want that behavior can comment them out.
     */
#if LV_TXT_ENC == LV_TXT_ENC_UTF8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Register custom event code
    //MINUTE_TICK = lv_event_register_id();

    int32_t zoom_level = 100;
    bool allow_dpi_override = false;
    bool simulator_mode = true;
    lv_display_t* display = lv_windows_create_display(
        L"LVGL Windows Simulator Display 1",
        800,
        480,
        zoom_level,
        allow_dpi_override,
        simulator_mode);
    if (!display)
    {
        return -1;
    }

    HWND window_handle = lv_windows_get_display_window_handle(display);
    if (!window_handle)
    {
        return -1;
    }

    HICON icon_handle = LoadIconW(
        GetModuleHandleW(NULL),
        MAKEINTRESOURCE(IDI_LVGL_WINDOWS));
    if (icon_handle)
    {
        SendMessageW(
            window_handle,
            WM_SETICON,
            TRUE,
            (LPARAM)icon_handle);
        SendMessageW(
            window_handle,
            WM_SETICON,
            FALSE,
            (LPARAM)icon_handle);
    }

    lv_indev_t* pointer_indev = lv_windows_acquire_pointer_indev(display);
    if (!pointer_indev)
    {
        return -1;
    }

    lv_indev_t* keypad_indev = lv_windows_acquire_keypad_indev(display);
    if (!keypad_indev)
    {
        return -1;
    }

    lv_indev_t* encoder_indev = lv_windows_acquire_encoder_indev(display);
    if (!encoder_indev)
    {
        return -1;
    }

    // Initialize UI components
    init_time_date_display();
    init_schedule_ui();

    // Create minute timer (check every second for minute change)
    minute_timer = lv_timer_create(minute_tick, 1000, NULL);

    // Initial update
    //lv_event_send(lv_screen_active(), MINUTE_TICK, NULL);

    while (1)
    {
        uint32_t time_till_next = lv_timer_handler();
        lv_delay_ms(time_till_next);

        // Check for window close
        //if (!lv_windows_is_running(display))
        //{
        //    break;
        //}
    }

    // Cleanup
    //time_display_deinit();
    //schedule_ui_deinit();
    //if (minute_timer) {
    //    lv_timer_del(minute_timer);
    //    minute_timer = NULL;
    //}
    //lv_obj_clean(lv_screen_active());
    //lv_indev_delete(pointer_indev);
    //lv_indev_delete(keypad_indev);
    //lv_indev_delete(encoder_indev);
    //lv_display_delete(display);
    //lv_deinit();

    //if (icon_handle)
    //{
    //    DestroyIcon(icon_handle);
    //}

    return 0;
}
