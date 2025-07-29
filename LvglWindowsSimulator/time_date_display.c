#include "time_date_display.h"
#include "locale.h"
#include <lvgl/lvgl.h>
#include <stdio.h>
#include <time.h>

static lv_obj_t* time_label;
static lv_obj_t* date_label;

void update_time_and_date_display(void)
{
    if (!time_label || !date_label) return;

    time_t now = time(NULL);
    struct tm* time = localtime(&now);

    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", time->tm_hour, time->tm_min);
    lv_label_set_text(time_label, time_str);

    char date_str[64];
    snprintf(date_str, sizeof(date_str), "%s, %d %s %d",
        days_of_week[time->tm_wday], time->tm_mday, months[time->tm_mon], time->tm_year + 1900);
    lv_label_set_text(date_label, date_str);
}

void init_time_and_date_display(void)
{
    // Time label
    time_label = lv_label_create(lv_screen_active());
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(time_label, "time not initialized");
    lv_obj_set_style_text_font(time_label, &lv_font_my_montserrat_20, 0);

    // Date label
    date_label = lv_label_create(lv_screen_active());
    lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, 80, 10);
    lv_label_set_text(date_label, "date not initialized");
    lv_obj_set_style_text_font(date_label, &lv_font_my_montserrat_20, 0);

    update_time_and_date_display();
}
