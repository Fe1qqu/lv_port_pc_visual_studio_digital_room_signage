#include "schedule_ui.h"
#include "schedule_data.h"
//#include "events.h"
#include <lvgl/lvgl.h>
#include <stdio.h>
#include <time.h>

static lv_obj_t* list_container;
static int current_lesson_index = 0;

void update_schedule_display(void)
{
    if (!list_container) return;

    // Clear existing content
    lv_obj_clean(list_container);

    // Get current time
    time_t now = time(NULL);
    struct tm* current_time = localtime(&now);
    int current_minutes = current_time->tm_hour * 60 + current_time->tm_min;

    // Get total number of lessons
    int lesson_count = get_lesson_count();

    // Create a block for each lesson
    for (int i = 0; i < lesson_count; i++)
    {
        lesson_t lesson = get_lesson(i);

        // Create block container
        lv_obj_t* block = lv_obj_create(list_container);
        lv_obj_set_size(block, 650, 150); // Block size
        lv_obj_set_pos(block, 0, i * 160); // Stack vertically with spacing
        lv_obj_set_style_bg_color(block, lv_color_hex(0xF0F0F0), 0); // Light background
        lv_obj_set_style_border_width(block, 1, 0);
        lv_obj_set_style_pad_all(block, 10, 0);

        // Progress bar
        lv_obj_t* progress_bar = lv_bar_create(block);
        lv_obj_set_size(progress_bar, 600, 20);
        lv_bar_set_range(progress_bar, 0, 100);
        lv_obj_align(progress_bar, LV_ALIGN_TOP_MID, 0, 0);
        //lv_obj_set_style_bg_color(progress_bar, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);

        // Calculate progress
        int start_minutes = lesson.start_hour * 60 + lesson.start_minute;
        int end_minutes = lesson.end_hour * 60 + lesson.end_minute;
        int progress = 0;
        if (current_minutes > end_minutes) // Lesson has ended, set progress to 100%
        {
            progress = 100;
        }
        else if (current_minutes >= start_minutes && current_minutes <= end_minutes) // Lesson is ongoing, calculate progress
        {
            progress = ((current_minutes - start_minutes) * 100) / (end_minutes - start_minutes);
        }
        lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);

        // Start time label
        lv_obj_t* start_time_label = lv_label_create(block);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", lesson.start_hour, lesson.start_minute);
        lv_label_set_text(start_time_label, buffer);
        lv_obj_align_to(start_time_label, progress_bar, LV_ALIGN_TOP_LEFT, 5, -2);
        lv_obj_set_style_text_font(start_time_label, &lv_font_my_montserrat_20, 0);

        // End time label
        lv_obj_t* end_time_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%02d:%02d", lesson.end_hour, lesson.end_minute);
        lv_label_set_text(end_time_label, buffer);
        lv_obj_align_to(end_time_label, progress_bar, LV_ALIGN_TOP_RIGHT, -18, -2);
        lv_obj_set_style_text_font(end_time_label, &lv_font_my_montserrat_20, 0);

        // Type label
        lv_obj_t* type_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%s", lesson.type);
        lv_label_set_text(type_label, buffer);
        lv_obj_align(type_label, LV_ALIGN_TOP_MID, 0, 30);
        lv_obj_set_style_text_font(type_label, &lv_font_my_montserrat_20, 0);

        // Subject label (SCROLL)
        lv_obj_t* subject_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%s", lesson.subject);
        lv_label_set_text(subject_label, buffer);
        lv_label_set_long_mode(subject_label, LV_LABEL_LONG_MODE_SCROLL);
        lv_obj_set_style_anim_time(subject_label, 3000, 0);
        lv_obj_set_width(subject_label, 600);
        lv_obj_align(subject_label, LV_ALIGN_TOP_MID, 0, 60);
        lv_obj_set_style_text_font(subject_label, &lv_font_my_montserrat_20, 0);

        // Teacher label
        lv_obj_t* teacher_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%s", lesson.teacher);
        lv_label_set_text(teacher_label, buffer);
        lv_obj_set_width(teacher_label, 600);
        lv_obj_align(teacher_label, LV_ALIGN_TOP_MID, 0, 90);
        lv_obj_set_style_text_font(teacher_label, &lv_font_my_montserrat_20, 0);

        // Highlight current lesson
        //if (i == current_lesson_index) {
        //    lv_obj_set_style_border_color(block, lv_color_hex(0x0000FF), 0); // Blue border for current lesson
        //    lv_obj_set_style_border_width(block, 3, 0);
        //}
    }

    // Scroll to current lesson
    //lv_obj_scroll_to_y(list_container, current_lesson_index * 160, LV_ANIM_ON);
}

static void gesture_event_cb(lv_event_t* e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    int lesson_count = get_lesson_count();

    if (dir == LV_DIR_TOP && current_lesson_index < lesson_count - 1)
    {
        current_lesson_index++;
        update_schedule_display();
    }
    else if (dir == LV_DIR_BOTTOM && current_lesson_index > 0)
    {
        current_lesson_index--;
        update_schedule_display();
    }
}

void init_schedule_ui(void)
{
    // Create main container
    list_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(list_container, 700, 400);
    lv_obj_center(list_container);

    lv_obj_set_scroll_dir(list_container, LV_DIR_VER); // Vertical scrolling only
    lv_obj_set_scrollbar_mode(list_container, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0xFFFFFF), 0); // White background
    lv_obj_add_flag(list_container, LV_OBJ_FLAG_SCROLLABLE);

    // Add gesture event handler
    lv_obj_add_event_cb(list_container, gesture_event_cb, LV_EVENT_GESTURE, NULL);

    // Add MINUTE_TICK event handler
    //lv_obj_add_event_cb(list_container, update_schedule_display, MINUTE_TICK, NULL);

    // Initial update
    //lv_event_send(list_container, MINUTE_TICK, NULL);
    update_schedule_display();
}
