#include "schedule_ui.h"
#include "schedule_data.h"
#include "locale.h"
#include <lvgl/lvgl.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

LV_IMG_DECLARE(calendar_icon_light)

#define MAX_NUMBER_OF_LESSONS 16
#define POPUP_DURATION_MS 3000

static lv_obj_t* list_container;
static lv_obj_t* progress_bars[MAX_NUMBER_OF_LESSONS]; // Store lessons progress bars
static struct tm current_display_date;

static lv_obj_t* calendar;
static lv_obj_t* calendar_container;
static lv_obj_t* calendar_background;
static lv_obj_t* date_container;
static lv_obj_t* date_label;
static lv_calendar_date_t highlighted_date; // Store the highlighted date

static lv_obj_t* popup;
static lv_timer_t* popup_timer;

static void popup_timer_cb(lv_timer_t* timer)
{
    if (popup)
    {
        lv_obj_delete(popup);
        popup = NULL;
    }
    lv_timer_delete(timer);
    popup_timer = NULL;
}

static void show_popup(const char* message)
{
    if (popup)
    {
        lv_obj_delete(popup);
        popup = NULL;
    }
    if (popup_timer)
    {
        lv_timer_delete(popup_timer);
        popup_timer = NULL;
    }

    popup = lv_obj_create(lv_screen_active());
    lv_obj_set_size(popup, 400, 30);
    lv_obj_center(popup);
    lv_obj_align(popup, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_border_color(popup, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(popup, 1, 0);
    lv_obj_remove_flag(popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(popup, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(popup, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* label = lv_label_create(popup);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_font(label, &lv_font_my_montserrat_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
    lv_obj_center(label);

    // Create a timer to close automatically
    popup_timer = lv_timer_create(popup_timer_cb, POPUP_DURATION_MS, NULL);
}

static void close_calendar_cb(lv_event_t* e)
{
    (void)e;

    lv_obj_add_flag(calendar_container, LV_OBJ_FLAG_HIDDEN);
    lv_calendar_set_month_shown(calendar, current_display_date.tm_year + 1900, current_display_date.tm_mon + 1);
}

static void date_container_cb(lv_event_t* e)
{
    (void)e;

    lv_obj_remove_flag(calendar_container, LV_OBJ_FLAG_HIDDEN);
}

static void calendar_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_calendar_date_t date;
        lv_calendar_get_pressed_date(calendar, &date);
        struct tm selected_date = { 0 };
        selected_date.tm_year = date.year - 1900;
        selected_date.tm_mon = date.month - 1;
        selected_date.tm_mday = date.day;

        mktime(&selected_date);
        update_schedule_display(&selected_date);
    }
}

void update_schedule_display(struct tm* display_date)
{
    if (!list_container || !display_date) return;

    // If the date is already displayed, do nothing
    if (current_display_date.tm_year == display_date->tm_year &&
        current_display_date.tm_mon == display_date->tm_mon &&
        current_display_date.tm_mday == display_date->tm_mday)
    {
        return;
    }

    // Get total number of lessons
    int lesson_count = get_lesson_count_for_date(display_date);

    if (lesson_count == 0)
    {
        show_popup("Нет занятий на выбранную дату");
        return;
    }

    char date_str[64];
    snprintf(date_str, sizeof(date_str), "%s, %d %s %d",
        days_of_week[display_date->tm_wday], display_date->tm_mday,
        months[display_date->tm_mon], display_date->tm_year + 1900);
    lv_label_set_text(date_label, date_str);

    // Move date_container to lv_screen_active before clearing
    lv_obj_set_parent(date_container, lv_screen_active());

    // Clear existing content
    lv_obj_clean(list_container);
    for (int i = 0; i < MAX_NUMBER_OF_LESSONS; i++)
    {
        progress_bars[i] = NULL;
    }

    // Return date_container to list_container
    lv_obj_set_parent(date_container, list_container);

    // Highlight the selected calendar date
    highlighted_date.year = display_date->tm_year + 1900;
    highlighted_date.month = display_date->tm_mon + 1;
    highlighted_date.day = display_date->tm_mday;
    lv_calendar_set_highlighted_dates(calendar, &highlighted_date, 1);
    
    // Store display date
    memcpy(&current_display_date, display_date, sizeof(struct tm));

    // Get current time
    time_t now = time(NULL);
    struct tm* current_time = localtime(&now);
    int current_minutes = current_time->tm_hour * 60 + current_time->tm_min;

    // Compare dates (ignoring time)
    int is_past_date = 0, is_future_date = 0;
    if (display_date->tm_year < current_time->tm_year ||
        (display_date->tm_year == current_time->tm_year && display_date->tm_mon < current_time->tm_mon) ||
        (display_date->tm_year == current_time->tm_year && display_date->tm_mon == current_time->tm_mon &&
         display_date->tm_mday < current_time->tm_mday))
    {
        is_past_date = 1;
    }
    else if (display_date->tm_year > current_time->tm_year ||
        (display_date->tm_year == current_time->tm_year && display_date->tm_mon > current_time->tm_mon) ||
        (display_date->tm_year == current_time->tm_year && display_date->tm_mon == current_time->tm_mon &&
         display_date->tm_mday > current_time->tm_mday))
    {
        is_future_date = 1;
    }

    // Create a block for each lesson
    for (int i = 0; i < lesson_count; i++)
    {
        lesson_t lesson = get_lesson_for_date(display_date, i);

        // Create block container
        lv_obj_t* block = lv_obj_create(list_container);
        lv_obj_set_size(block, lv_pct(98), LV_SIZE_CONTENT);
        //lv_obj_set_style_bg_color(block, lv_color_hex(0xF0F0F0), 0);
        lv_obj_set_style_border_width(block, 1, 0);
        lv_obj_set_style_border_color(block, lv_color_hex(0x525252), 0);
        lv_obj_set_style_radius(block, 0, 0);
        lv_obj_remove_flag(block, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(block, LV_DIR_NONE);
        lv_obj_set_scrollbar_mode(block, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_layout(block, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(block, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(block, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Progress bar with striped pattern
        //LV_IMAGE_DECLARE(img_skew_strip);
        //static lv_style_t style_indic;
        //lv_style_init(&style_indic);
        //lv_style_set_bg_image_src(&style_indic, &img_skew_strip);
        //lv_style_set_bg_image_tiled(&style_indic, true);
        //lv_style_set_bg_image_opa(&style_indic, LV_OPA_30);

        //static lv_style_t style_main;
        //lv_style_init(&style_main);
        //lv_style_set_bg_image_src(&style_main, &img_skew_strip);
        //lv_style_set_bg_image_tiled(&style_main, true);
        //lv_style_set_bg_image_opa(&style_main, LV_OPA_50);

        //lv_obj_t* progress_bar = lv_bar_create(block);
        //lv_obj_add_style(progress_bar, &style_indic, LV_PART_INDICATOR);
        //lv_obj_add_style(progress_bar, &style_main, LV_PART_MAIN);
        //lv_obj_set_size(progress_bar, 600, 30);
        //lv_bar_set_range(progress_bar, 0, 100);
        //lv_obj_set_style_radius(progress_bar, 0, LV_PART_MAIN);
        //lv_obj_set_style_radius(progress_bar, 0, LV_PART_INDICATOR);
        //lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x90EE90), LV_PART_MAIN); // Fallback green
        //lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x228B22), LV_PART_INDICATOR); // Fallback green
        ////lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xCCCCCC), 0);
        //progress_bars[i] = progress_bar;

        // Progress bar
        lv_obj_t* progress_bar = lv_bar_create(block);
        lv_obj_set_size(progress_bar, lv_pct(100), 30);
        lv_bar_set_range(progress_bar, 0, 100);
        lv_obj_set_style_radius(progress_bar, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(progress_bar, 0, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x90EE90), LV_PART_MAIN);
        lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x228B22), LV_PART_INDICATOR);
        progress_bars[i] = progress_bar;

        // Calculate progress
        int progress = 0;
        if (is_past_date)
        {
            progress = 100;
        }
        else if (is_future_date)
        {
            progress = 0;
        }
        else
        {
            int start_minutes = lesson.start_hour * 60 + lesson.start_minute;
            int end_minutes = lesson.end_hour * 60 + lesson.end_minute;
            if (current_minutes > end_minutes)
            {
                progress = 100;
            }
            else if (current_minutes >= start_minutes && current_minutes <= end_minutes)
            {
                progress = ((current_minutes - start_minutes) * 100) / (end_minutes - start_minutes);
            }
        }
        lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);

        char buffer[6];

        // Start time label
        lv_obj_t* start_time_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%02d:%02d", lesson.start_hour, lesson.start_minute);
        lv_label_set_text(start_time_label, buffer);
        lv_obj_set_style_text_font(start_time_label, &lv_font_my_montserrat_20, 0);
        lv_obj_set_style_text_color(start_time_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_align(start_time_label, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_add_flag(start_time_label, LV_OBJ_FLAG_FLOATING);
        lv_obj_align_to(start_time_label, progress_bar, LV_ALIGN_LEFT_MID, 5, -1);

        // End time label
        lv_obj_t* end_time_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%02d:%02d", lesson.end_hour, lesson.end_minute);
        lv_label_set_text(end_time_label, buffer);
        lv_obj_set_style_text_font(end_time_label, &lv_font_my_montserrat_20, 0);
        lv_obj_set_style_text_color(end_time_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_align(end_time_label, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_add_flag(end_time_label, LV_OBJ_FLAG_FLOATING);
        lv_obj_align_to(end_time_label, progress_bar, LV_ALIGN_RIGHT_MID, -5, -1);

        // Type label
        lv_obj_t* type_label = lv_label_create(block);
        lv_label_set_text(type_label, lesson.type);
        lv_obj_set_width(type_label, lv_pct(100));
        lv_obj_set_style_text_font(type_label, &lv_font_my_montserrat_20, 0);
        lv_obj_set_style_text_color(type_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_align(type_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_bg_opa(type_label, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(type_label, 5, 0);
        lv_obj_set_style_bg_color(type_label, lv_color_hex(lesson.color), 0);
        
        // Subject label (WRAP)
        lv_obj_t* subject_label = lv_label_create(block);
        lv_label_set_text(subject_label, lesson.subject);
        lv_label_set_long_mode(subject_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(subject_label, lv_pct(100));
        lv_obj_set_style_text_font(subject_label, &lv_font_my_montserrat_20, 0);
        lv_obj_set_style_text_color(subject_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_pad_top(subject_label, 5, 0);
        lv_obj_set_style_pad_bottom(subject_label, 10, 0);

        // Dashed line
        lv_coord_t block_width = lv_obj_get_width(block);
        static lv_point_precise_t line_points[2];
        line_points[0] = (lv_point_precise_t){ 0, 0 };
        line_points[1] = (lv_point_precise_t){ block_width, 0 };
        lv_obj_t* line = lv_line_create(block);
        lv_line_set_points(line, line_points, 2);
        lv_obj_set_style_line_color(line, lv_color_hex(0x000000), 0);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_dash_width(line, 2, 0);
        lv_obj_set_style_line_dash_gap(line, 2, 0);
        lv_obj_set_width(line, lv_pct(100));

        // Labels container for teacher and groups
        lv_obj_t* labels_container = lv_obj_create(block);
        lv_obj_set_size(labels_container, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(labels_container, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(labels_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(labels_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(labels_container, 0, 0);
        lv_obj_set_style_border_width(labels_container, 0, 0);
        lv_obj_set_style_bg_opa(labels_container, LV_OPA_TRANSP, 0);

        // Teacher label
        lv_obj_t* teacher_label = lv_label_create(labels_container);
        lv_label_set_text(teacher_label, lesson.teacher);
        lv_label_set_long_mode(teacher_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(teacher_label, lv_pct(45));
        lv_obj_set_style_text_font(teacher_label, &lv_font_my_montserrat_20, 0);
        lv_obj_set_style_text_color(teacher_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_align(teacher_label, LV_TEXT_ALIGN_LEFT, 0);

        // Groups label
        lv_obj_t* groups_label = lv_label_create(labels_container);
        lv_label_set_text(groups_label, lesson.groups);
        lv_label_set_long_mode(groups_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(groups_label, lv_pct(45));
        lv_obj_set_style_text_font(groups_label, &lv_font_my_montserrat_20, 0);
        lv_obj_set_style_text_color(groups_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_align(groups_label, LV_TEXT_ALIGN_RIGHT, 0);
    }
}

void update_progress_bar(void)
{
    if (!list_container) return;

    // Get total number of lessons
    int lesson_count = get_lesson_count();
    if (lesson_count == 0) return;

    // Get current time
    time_t now = time(NULL);
    struct tm* current_time = localtime(&now);
    int current_minutes = current_time->tm_hour * 60 + current_time->tm_min;

    // Check if displayed date is today
    int is_today = (current_display_date.tm_year == current_time->tm_year &&
                    current_display_date.tm_mon == current_time->tm_mon &&
                    current_display_date.tm_mday == current_time->tm_mday);
    if (!is_today) return;

    // Update progress bar of current lesson
    for (int i = 0; i < lesson_count; i++)
    {
        if (progress_bars[i])
        {
            lesson_t lesson = get_lesson(i);
            int start_minutes = lesson.start_hour * 60 + lesson.start_minute;
            int end_minutes = lesson.end_hour * 60 + lesson.end_minute;
            int progress = 0;
            if (current_minutes > end_minutes)
            {
                progress = 100;
            }
            else if (current_minutes >= start_minutes && current_minutes <= end_minutes)
            {
                progress = ((current_minutes - start_minutes) * 100) / (end_minutes - start_minutes);
            }
            lv_bar_set_value(progress_bars[i], progress, LV_ANIM_ON);
        }
    }
}

void init_schedule_ui(void)
{
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFFFFFF), 0); // White background

    // Create main container
    list_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(list_container, lv_pct(100), lv_pct(90));
    lv_obj_align(list_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_scroll_dir(list_container, LV_DIR_VER); // Vertical scrolling only
    lv_obj_set_scrollbar_mode(list_container, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0xFFFFFF), 0); // White background
    //lv_obj_set_style_border_width(list_container, 0, 0);
    lv_obj_set_style_radius(list_container, 0, 0);
    lv_obj_add_flag(list_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(list_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(list_container, 15, 0);

    // Create date container
    date_container = lv_obj_create(list_container);
    lv_obj_set_size(date_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(date_container, lv_color_hex(0xFFFFFF), 0); // White background
    lv_obj_set_style_border_width(date_container, 0, 0);
    lv_obj_set_layout(date_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(date_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(date_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(date_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(date_container, date_container_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_remove_flag(date_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(date_container, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(date_container, LV_SCROLLBAR_MODE_OFF);

    // Calendar date
    date_label = lv_label_create(date_container);
    lv_label_set_text(date_label, "Расписание отсутствует");
    lv_obj_set_style_text_font(date_label, &lv_font_my_montserrat_20, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x000000), 0);

    // Calendar icon
    lv_obj_t* calendar_icon = lv_image_create(date_container);
    lv_image_set_src(calendar_icon, &calendar_icon_light);
    lv_obj_set_style_pad_left(calendar_icon, 5, 0);
    lv_obj_set_style_translate_y(calendar_icon, -2, 0);

    // Сalendar container
    calendar_container = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(calendar_container);
    lv_obj_set_size(calendar_container, lv_pct(100), lv_pct(100));
    lv_obj_center(calendar_container);
    lv_obj_add_flag(calendar_container, LV_OBJ_FLAG_HIDDEN);

    // Darkened background
    calendar_background = lv_obj_create(calendar_container);
    lv_obj_remove_style_all(calendar_background);
    lv_obj_set_size(calendar_background, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(calendar_background, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(calendar_background, LV_OPA_50, 0);
    lv_obj_add_flag(calendar_background, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(calendar_background, close_calendar_cb, LV_EVENT_CLICKED, NULL);

    // Create calendar
    calendar = lv_calendar_create(calendar_container);
    lv_obj_set_size(calendar, 300, 350);
    lv_obj_center(calendar);

    // Create header with arrows
    lv_obj_t* header = lv_calendar_header_arrow_create(calendar);

    // Style for the left arrow (child 0)
    lv_obj_t* left_arrow = lv_obj_get_child(header, 0);
    lv_obj_set_style_bg_opa(left_arrow, LV_OPA_TRANSP, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_bg_opa(left_arrow, LV_OPA_TRANSP, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_PRESSED));
    lv_obj_set_style_border_width(left_arrow, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_shadow_width(left_arrow, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_text_color(left_arrow, lv_color_hex(0x000000), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT)); // Arrow color

    // Style for the right arrow (child 2)
    lv_obj_t* right_arrow = lv_obj_get_child(header, 2);
    lv_obj_set_style_bg_opa(right_arrow, LV_OPA_TRANSP, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_bg_opa(right_arrow, LV_OPA_TRANSP, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_PRESSED));
    lv_obj_set_style_border_width(right_arrow, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_shadow_width(right_arrow, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_text_color(right_arrow, lv_color_hex(0x000000), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT)); // Arrow color

    lv_obj_add_event_cb(calendar, calendar_event_cb, LV_EVENT_ALL, NULL);

    // Close button
    lv_obj_t* close_button = lv_button_create(calendar);
    lv_obj_set_size(close_button, lv_pct(100), 40);
    lv_obj_set_style_margin_all(close_button, 8, 0);
    lv_obj_t* close_label = lv_label_create(close_button);
    lv_label_set_text(close_label, "Закрыть");
    lv_obj_set_style_text_font(close_label, &lv_font_my_montserrat_20, 0);
    lv_obj_center(close_label);
    lv_obj_add_event_cb(close_button, close_calendar_cb, LV_EVENT_CLICKED, NULL);

    // Initial update (current date)
    time_t now = time(NULL);
    struct tm* current_date = localtime(&now);
    memcpy(&current_display_date, current_date, sizeof(struct tm));
    lv_calendar_set_month_shown(calendar, current_date->tm_year + 1900, current_date->tm_mon + 1);
    update_schedule_display(current_date);
}
