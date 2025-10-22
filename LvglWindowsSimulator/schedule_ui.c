#include "schedule_ui.h"
#include "schedule_data.h"
#include "locale.h"
#include "config.h"
#include <lvgl/lvgl.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

LV_IMG_DECLARE(calendar_icon)
LV_IMG_DECLARE(theme_icon_light)
LV_IMG_DECLARE(theme_icon_dark)

#define MAX_NUMBER_OF_LESSONS 10
#define POPUP_DURATION_MS 3000

static lv_obj_t* list_container;
static lv_obj_t* blocks[MAX_NUMBER_OF_LESSONS]; // Store lessons blocks
static struct tm current_display_date;
static struct tm start_academic_date;
static struct tm end_academic_date;

static lv_obj_t* calendar;
static lv_obj_t* calendar_header;
static lv_obj_t* calendar_close_button;
static lv_obj_t* calendar_container;
static lv_obj_t* calendar_background;
static lv_style_t day_style; // Style for calendar day buttons
static lv_obj_t* date_label;
static lv_obj_t* calendar_image;
static lv_obj_t* clickable_container; // Container for clickable area to open calendar
static lv_calendar_date_t highlighted_date; // Store the highlighted date

static lv_obj_t* popup;
static lv_timer_t* popup_timer;

static lv_obj_t* theme_toggle_button;
static bool is_dark_theme = false;
static uint32_t inactive_duration_ms = 60000;

void set_dark_theme(bool is_dark)
{
    is_dark_theme = is_dark;
}

void set_inactive_duration(uint32_t duration_ms)
{
    inactive_duration_ms = duration_ms;
}

static void inactivity_check_cb(lv_timer_t* timer)
{
    (void)timer;
    lv_display_t* display = lv_display_get_default();
    if (!display) return;

    uint32_t inactive_time_ms = lv_display_get_inactive_time(display);
    if (inactive_time_ms >= inactive_duration_ms)
    {
        // Get current time
        time_t now = time(NULL);
        struct tm* current_time = localtime(&now);

        update_schedule_display(current_time);
    }
    //printf("Inactivity check: %d ms\n", inactive_time_ms);
}

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
    lv_obj_align(popup, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_border_color(popup, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(popup, 1, 0);
    lv_obj_set_style_radius(popup, 0, 0);
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

static void style_progress_bar_and_labels(lv_obj_t* progress_bar, lv_obj_t* start_time_label, lv_obj_t* end_time_label, int progress)
{
    if (is_dark_theme)
    {
        lv_obj_set_style_text_color(start_time_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(end_time_label, lv_color_hex(0xFFFFFF), 0);
        if (progress == 100)
        {
            lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x276f2f), LV_PART_INDICATOR);
        }
        else
        {
            lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x477285), LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
    }
    else
    {
        if (progress == 100)
        {
            lv_obj_set_style_text_color(start_time_label, lv_color_hex(0x276f2f), 0);
            lv_obj_set_style_text_color(end_time_label, lv_color_hex(0x276f2f), 0);
            lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x9ffea5), LV_PART_INDICATOR);
        }
        else
        {
            lv_obj_set_style_text_color(start_time_label, lv_color_hex(0x285886), 0);
            lv_obj_set_style_text_color(end_time_label, lv_color_hex(0x285886), 0);
            lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xaddff6), LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x858585), LV_PART_MAIN);
        }
    }
}

static void update_calendar_arrow_state(lv_obj_t* calendar)
{
    const lv_calendar_date_t* showed_date = lv_calendar_get_showed_date(calendar);
    int year = showed_date->year;
    int month = showed_date->month;

    struct tm shown_date = { 0 };
    shown_date.tm_year = year - 1900;
    shown_date.tm_mon = month - 1;
    shown_date.tm_mday = 1;
    mktime(&shown_date);

    struct tm prev_month = shown_date;
    prev_month.tm_mon -= 1;
    mktime(&prev_month);

    struct tm next_month = shown_date;
    next_month.tm_mon += 1;
    mktime(&next_month);

    lv_obj_t* left_arrow = lv_obj_get_child(calendar_header, 0);
    lv_obj_t* right_arrow = lv_obj_get_child(calendar_header, 2);

    // Enable/Disable left arrow
    if (mktime(&prev_month) < mktime(&start_academic_date))
    {
        //printf("Left arrow disabled\n");
        lv_obj_set_style_text_color(left_arrow, is_dark_theme ? lv_color_hex(0x000000) : lv_color_hex(0xBBBBBB), 0); // Disabled arrow color
        lv_obj_add_state(left_arrow, LV_STATE_DISABLED);
    }
    else
    {
        //printf("Left arrow enabled\n");
        lv_obj_set_style_text_color(left_arrow, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x2C72A5), 0); // Enabled arrow color
        lv_obj_remove_state(left_arrow, LV_STATE_DISABLED);
    }

    // Enable/Disable right arrow
    if (mktime(&next_month) > mktime(&end_academic_date))
    {
        //printf("Right arrow disabled\n");
        lv_obj_set_style_text_color(right_arrow, is_dark_theme ? lv_color_hex(0x000000) : lv_color_hex(0xBBBBBB), 0); // Disabled arrow color
        lv_obj_add_state(right_arrow, LV_STATE_DISABLED);
    }
    else
    {
        //printf("Right arrow enabled\n");
        lv_obj_set_style_text_color(right_arrow, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x2C72A5), 0); // Enabled arrow color
        lv_obj_remove_state(right_arrow, LV_STATE_DISABLED);
    }
}

static void close_calendar_cb(lv_event_t* event)
{
    (void)event;

    lv_obj_add_flag(calendar_container, LV_OBJ_FLAG_HIDDEN);

    if (current_display_date.tm_year == 0/* && current_display_date.tm_mon == 0 && current_display_date.tm_mday == 0*/)
    {
        // If no date is selected, show today's date
        time_t now = time(NULL);
        struct tm* current_date = localtime(&now);
        lv_calendar_set_month_shown(calendar, current_date->tm_year + 1900, current_date->tm_mon + 1);

    }
    else
    {
        // Update the calendar to show the currently displayed date
        lv_calendar_set_month_shown(calendar, current_display_date.tm_year + 1900, current_display_date.tm_mon + 1);
    }

    update_calendar_arrow_state(calendar);
}

static void calendar_container_cb(lv_event_t* event)
{
    (void)event;

    lv_obj_remove_flag(calendar_container, LV_OBJ_FLAG_HIDDEN);
}

static void prev_event_cb(lv_event_t* event)
{
    lv_obj_t* calendar = lv_event_get_user_data(event);

    update_calendar_arrow_state(calendar);
}

static void next_event_cb(lv_event_t* event)
{
    lv_obj_t* calendar = lv_event_get_user_data(event);

    update_calendar_arrow_state(calendar);
}

static void calendar_event_cb(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
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

static void toggle_theme_cb(lv_event_t* event)
{
    (void)event;

    is_dark_theme = !is_dark_theme;

    // Update screen background
    lv_obj_set_style_bg_color(lv_screen_active(), is_dark_theme ? lv_color_hex(0x303336) : lv_color_hex(0x2C72A5), 0);

    // Update list_container
    lv_obj_set_style_bg_color(list_container, is_dark_theme ? lv_color_hex(0x101012) : lv_color_hex(0xFFFFFF), 0);

    int lesson_count = get_lesson_count();

    // Update blocks
    for (int i = 0; i < lesson_count; i++)
    {
        if (blocks[i])
        {
            lv_obj_set_style_bg_color(blocks[i], is_dark_theme ? lv_color_hex(0x000000) : lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_color(lv_obj_get_child(blocks[i], 4), is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0); // Subject label
            lv_obj_set_style_line_color(lv_obj_get_child(blocks[i], 5), is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0); // Dashed line
            lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(blocks[i], 6), 0), is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0); // Teacher label
            lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(blocks[i], 6), 1), is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0); // Groups label

            // Update progress bar and labels colors based on current progress
            lv_obj_t* progress_bar = lv_obj_get_child(blocks[i], 0);
            lv_obj_t* start_time_label = lv_obj_get_child(blocks[i], 1);
            lv_obj_t* end_time_label = lv_obj_get_child(blocks[i], 2);
            int progress = lv_bar_get_value(progress_bar);
            style_progress_bar_and_labels(progress_bar, start_time_label, end_time_label, progress);
        }
    }

    // Update date_label
    lv_obj_set_style_text_color(date_label, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x2C72A5), 0);

    // Update calendar
    lv_obj_set_style_bg_color(calendar, is_dark_theme ? lv_color_hex(0x303336) : lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(calendar, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0);

    // Update calendar header arrows
    update_calendar_arrow_state(calendar);

    // Update calendar close button
    lv_obj_set_style_bg_color(calendar_close_button, is_dark_theme ? lv_color_hex(0x272727) : lv_color_hex(0x407AB2), 0);

    // Update toggle button icon
    lv_imagebutton_set_src(theme_toggle_button, LV_IMAGEBUTTON_STATE_RELEASED, is_dark_theme ? &theme_icon_dark : &theme_icon_light,
        is_dark_theme ? &theme_icon_dark : &theme_icon_light, NULL);
}

static void clear_chedule_content()
{
    // Move and date_label to lv_screen_active before clearing
    lv_obj_set_parent(date_label, lv_screen_active());

    // Clear existing content
    lv_obj_clean(list_container);
    for (int i = 0; i < MAX_NUMBER_OF_LESSONS; i++)
    {
        blocks[i] = NULL;
    }

    // Return and date_label to list_container
    lv_obj_set_parent(date_label, list_container);
}

static void highlight_calendar_date(struct tm* display_date)
{
    // Highlight the selected calendar date
    highlighted_date.year = display_date->tm_year + 1900;
    highlighted_date.month = display_date->tm_mon + 1;
    highlighted_date.day = display_date->tm_mday;
    lv_calendar_set_highlighted_dates(calendar, &highlighted_date, 1);
}

void update_schedule_display(struct tm* display_date)
{
    if (!list_container || !date_label || !display_date) return;

    // If the date is already displayed, do nothing
    if (current_display_date.tm_year == display_date->tm_year &&
        current_display_date.tm_mon == display_date->tm_mon &&
        current_display_date.tm_mday == display_date->tm_mday)
    {
        return;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm* current_time = localtime(&now);

    bool is_today = (display_date->tm_year == current_time->tm_year &&
                     display_date->tm_mon == current_time->tm_mon &&
                     display_date->tm_mday == current_time->tm_mday);

    // Get total number of lessons
    int lesson_count = get_lesson_count_for_date(display_date);

    if (is_today && lesson_count == 0)
    {
        lv_label_set_text(date_label, "На сегодня занятий нет");
        clear_chedule_content();
        highlight_calendar_date(display_date);
        memcpy(&current_display_date, display_date, sizeof(struct tm));
        close_calendar_cb(NULL);
        return;
    }
    else if (!is_today && lesson_count == 0)
    {
        show_popup("Нет занятий на выбранную дату");
        return;
    }

    char date_str[64];
    snprintf(date_str, sizeof(date_str), "%s, %d %s %d",
        days_of_week[display_date->tm_wday], display_date->tm_mday,
        months[display_date->tm_mon], display_date->tm_year + 1900);
    lv_label_set_text(date_label, date_str);

    clear_chedule_content();
    highlight_calendar_date(display_date);
    memcpy(&current_display_date, display_date, sizeof(struct tm));
    close_calendar_cb(NULL);

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
        lv_obj_set_style_bg_color(block, is_dark_theme ? lv_color_hex(0x000000) : lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(block, 1, 0);
        lv_obj_set_style_border_color(block, lv_color_hex(0x525252), 0);
        lv_obj_set_style_radius(block, 0, 0);
        lv_obj_remove_flag(block, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(block, LV_DIR_NONE);
        lv_obj_set_scrollbar_mode(block, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_layout(block, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(block, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(block, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        blocks[i] = block;

        // Progress bar
        lv_obj_t* progress_bar = lv_bar_create(block);
        lv_obj_set_size(progress_bar, lv_pct(100), 30);
        lv_bar_set_range(progress_bar, 0, 100);
        lv_obj_set_style_radius(progress_bar, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(progress_bar, 0, LV_PART_INDICATOR);

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
        lv_obj_set_style_text_align(start_time_label, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_add_flag(start_time_label, LV_OBJ_FLAG_FLOATING);
        lv_obj_align_to(start_time_label, progress_bar, LV_ALIGN_LEFT_MID, 5, -1);

        // End time label
        lv_obj_t* end_time_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%02d:%02d", lesson.end_hour, lesson.end_minute);
        lv_label_set_text(end_time_label, buffer);
        lv_obj_set_style_text_font(end_time_label, &lv_font_my_montserrat_20, 0);
        lv_obj_set_style_text_align(end_time_label, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_add_flag(end_time_label, LV_OBJ_FLAG_FLOATING);
        lv_obj_align_to(end_time_label, progress_bar, LV_ALIGN_RIGHT_MID, -5, -1);

        // Set colors for progress bar and labels
        style_progress_bar_and_labels(progress_bar, start_time_label, end_time_label, progress);

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
        lv_obj_set_style_text_color(subject_label, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0);
        lv_obj_set_style_pad_top(subject_label, 5, 0);
        lv_obj_set_style_pad_bottom(subject_label, 10, 0);

        // Dashed line
        lv_coord_t block_width = lv_obj_get_width(block);
        static lv_point_precise_t line_points[2];
        line_points[0] = (lv_point_precise_t){ 0, 0 };
        line_points[1] = (lv_point_precise_t){ block_width, 0 };
        lv_obj_t* line = lv_line_create(block);
        lv_line_set_points(line, line_points, 2);
        lv_obj_set_style_line_color(line, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0);
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
        lv_obj_set_style_text_color(teacher_label, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_align(teacher_label, LV_TEXT_ALIGN_LEFT, 0);

        // Groups label
        lv_obj_t* groups_label = lv_label_create(labels_container);
        lv_label_set_text(groups_label, lesson.groups);
        lv_label_set_long_mode(groups_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(groups_label, lv_pct(45));
        lv_obj_set_style_text_font(groups_label, &lv_font_my_montserrat_20, 0);
        lv_obj_set_style_text_color(groups_label, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0);
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
        if (blocks[i])
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

            lv_obj_t* progress_bar = lv_obj_get_child(blocks[i], 0);
            lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);

            // Update colors based on new progress
            lv_obj_t* start_time_label = lv_obj_get_child(blocks[i], 1);
            lv_obj_t* end_time_label = lv_obj_get_child(blocks[i], 2);
            style_progress_bar_and_labels(progress_bar, start_time_label, end_time_label, progress);
        }
    }
}

void init_schedule_ui(void)
{
    lv_obj_set_style_bg_color(lv_screen_active(), is_dark_theme ? lv_color_hex(0x303336) : lv_color_hex(0x2C72A5), 0);

    // Create theme toggle button
    theme_toggle_button = lv_imagebutton_create(lv_screen_active());
    lv_imagebutton_set_src(theme_toggle_button, LV_IMAGEBUTTON_STATE_RELEASED,
        is_dark_theme ? &theme_icon_dark : &theme_icon_light,
        is_dark_theme ? &theme_icon_dark : &theme_icon_light, NULL);
    lv_obj_set_size(theme_toggle_button, 32, 32);
    lv_obj_align(theme_toggle_button, LV_ALIGN_TOP_RIGHT, -10, 8);
    lv_obj_add_event_cb(theme_toggle_button, toggle_theme_cb, LV_EVENT_CLICKED, NULL);

    // Calendar image
    calendar_image = lv_image_create(lv_screen_active());
    lv_image_set_src(calendar_image, &calendar_icon);
    lv_obj_align_to(calendar_image, theme_toggle_button, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_add_flag(calendar_image, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(calendar_image, calendar_container_cb, LV_EVENT_CLICKED, NULL);

    // Create clickable container over time, date, and calendar icon
    clickable_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(clickable_container, 435, 48);
    lv_obj_align(clickable_container, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(clickable_container, LV_OPA_TRANSP, 0); // Transparent background
    lv_obj_set_style_border_width(clickable_container, 0, 0);
    lv_obj_add_flag(clickable_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(clickable_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(clickable_container, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(clickable_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(clickable_container, calendar_container_cb, LV_EVENT_CLICKED, NULL);

    // Create list container
    list_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(list_container, lv_pct(100), lv_pct(94));
    lv_obj_align(list_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_scroll_dir(list_container, LV_DIR_VER); // Vertical scrolling only
    lv_obj_set_scrollbar_mode(list_container, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_bg_color(list_container, is_dark_theme ? lv_color_hex(0x101012) : lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(list_container, 0, 0);
    lv_obj_set_style_radius(list_container, 0, 0);
    lv_obj_add_flag(list_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(list_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(list_container, 15, 0);

    // Calendar date
    date_label = lv_label_create(list_container);
    lv_label_set_text(date_label, "На сегодня занятий нет");
    lv_obj_set_style_text_font(date_label, &lv_font_my_montserrat_20, 0);
    lv_obj_set_style_text_color(date_label, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x2C72A5), 0);
    lv_obj_set_style_pad_all(date_label, 5, 0);

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
    lv_obj_set_style_bg_color(calendar, is_dark_theme ? lv_color_hex(0x303336) : lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(calendar, is_dark_theme ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(calendar, 0, 0);

    // Change calendar day buttons style
    lv_obj_t* btnmatrix = lv_calendar_get_btnmatrix(calendar);
    if (btnmatrix)
    {
        lv_style_init(&day_style);
        lv_style_set_radius(&day_style, 2);
        lv_style_set_border_width(&day_style, 0);
        lv_style_set_text_font(&day_style, &lv_font_my_montserrat_14);
        lv_obj_add_style(btnmatrix, &day_style, LV_PART_ITEMS);
    }

    // Create header with arrows
    calendar_header = lv_calendar_header_arrow_create(calendar);

    // Style for the left arrow (child 0)
    lv_obj_t* left_arrow = lv_obj_get_child(calendar_header, 0);
    lv_obj_set_style_bg_opa(left_arrow, LV_OPA_TRANSP, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_bg_opa(left_arrow, LV_OPA_TRANSP, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_PRESSED));
    lv_obj_set_style_border_width(left_arrow, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_shadow_width(left_arrow, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

    // Style for header (year and month) (child 1)
    lv_obj_set_style_text_font(lv_obj_get_child(calendar_header, 1), &lv_font_my_montserrat_14, 0);

    // Style for the right arrow (child 2)
    lv_obj_t* right_arrow = lv_obj_get_child(calendar_header, 2);
    lv_obj_set_style_bg_opa(right_arrow, LV_OPA_TRANSP, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_bg_opa(right_arrow, LV_OPA_TRANSP, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_PRESSED));
    lv_obj_set_style_border_width(right_arrow, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));
    lv_obj_set_style_shadow_width(right_arrow, 0, (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_DEFAULT));

    lv_obj_add_event_cb(calendar, calendar_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(lv_obj_get_child(calendar_header, 0), prev_event_cb, LV_EVENT_CLICKED, calendar);
    lv_obj_add_event_cb(lv_obj_get_child(calendar_header, 2), next_event_cb, LV_EVENT_CLICKED, calendar);

    // Сalendar сlose button
    calendar_close_button = lv_button_create(calendar);
    lv_obj_set_size(calendar_close_button, lv_pct(100), 40);
    lv_obj_set_style_bg_color(calendar_close_button, is_dark_theme ? lv_color_hex(0x272727) : lv_color_hex(0x407AB2), 0);
    lv_obj_set_style_margin_all(calendar_close_button, 8, 0);
    lv_obj_t* close_label = lv_label_create(calendar_close_button);
    lv_label_set_text(close_label, "Закрыть");
    lv_obj_set_style_text_font(close_label, &lv_font_my_montserrat_20, 0);
    lv_obj_center(close_label);
    lv_obj_add_event_cb(calendar_close_button, close_calendar_cb, LV_EVENT_CLICKED, NULL);

    // Initial update (current date)
    time_t now = time(NULL);
    struct tm* current_date = localtime(&now);

    // Set academic year boundaries
    if (current_date->tm_mon < 8) // January–August → previous September
    {
        start_academic_date.tm_year = current_date->tm_year - 1;
    }
    else
    {
        start_academic_date.tm_year = current_date->tm_year;
    }

    start_academic_date.tm_mon = 8; // September
    start_academic_date.tm_mday = 1;
    mktime(&start_academic_date);

    end_academic_date = start_academic_date;
    end_academic_date.tm_year += 1;
    end_academic_date.tm_mon = 6; // July
    //end_academic_date.tm_mon = 10; // DEBUG
    end_academic_date.tm_mday = 31;
    mktime(&end_academic_date);

    lv_timer_create(inactivity_check_cb, 1000, NULL);

    update_schedule_display(current_date);
    lv_calendar_set_month_shown(calendar, current_date->tm_year + 1900, current_date->tm_mon + 1);
    update_calendar_arrow_state(calendar);
}
