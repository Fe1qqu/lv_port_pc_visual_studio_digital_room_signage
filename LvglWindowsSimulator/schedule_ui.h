#ifndef SCHEDULE_UI_H
#define SCHEDULE_UI_H

#include <stdbool.h>
#include <stdint.h>

struct tm;

/**
 * Initializes the schedule user interface.
 * @note Must be called after lvgl initialization and before any UI updates.
 */
void init_schedule_ui(void);

/**
 * Updates the schedule display for a specified date.
 * Clears the existing schedule UI and rebuilds it with lesson data for the given date.
 * @param date  Pointer to a struct tm containing the date to display (year, month, day).
 */
void update_schedule_display(struct tm* date);

/**
 * Updates the progress bar for the current lesson.
 * Updates only the progress bar of the active lesson (if any) for the current date, based on the current time.
 * Skips updates for non-current dates or if no lesson is active.
 * @note Should be called periodically (e.g., every minute) to reflect real-time progress.
 */
void update_progress_bar(void);

/**
 * Set the ui theme state
 * @param is_dark Boolean value:
 *        - `true` — enable dark theme
 *        - `false` — disable dark theme
 */
void set_dark_theme(bool is_dark);

/**
 * Sets the inactive duration timeout.
 * @param duration_ms Duration of inactivity in milliseconds.
 */
void set_inactive_duration(uint32_t duration_ms);

#endif
