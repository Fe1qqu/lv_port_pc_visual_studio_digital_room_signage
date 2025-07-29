#include "api.h"
#include "cJSON.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Callback for libcurl
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    char** response = (char**)userp;
    size_t oldsize = *response ? strlen(*response) : 0;
    char* new_response = realloc(*response, oldsize + realsize + 1);
    if (!new_response)
    {
        fprintf(stderr, "Failed to realloc response buffer\n");
        return 0;
    }
    *response = new_response;
    memcpy(*response + oldsize, contents, realsize);
    (*response)[oldsize + realsize] = '\0';
    return realsize;
}

// Dictionary of lesson types and their background colors
static const struct {
    const char* api_type;
    const char* display_type;
    uint32_t color;
} type_colors[] = {
    {"лек.", "Лекции", 0x276093},
    {"практ.зан.  и семин.", "Практические занятия и семинары", 0xff8f00},
    {"лаб.", "Лабораторные занятия", 0x3e8470},
    {"зач.", "Зачет", 0xe91e63},
    {"экз.", "Экзамен", 0xe91e63},
    {"диф.зач.", "Дифференцированный зачет", 0xe91e63},
    {"конс.эк.", "Консультация к промежуточной аттестации", 0x9e5fa1},
    {"-", "-", 0x407ab2},
    {"курс.раб.", "Курсовая работа", 0x407ab2},
    {NULL, NULL, 0xCCCCCC}
};

int fetch_schedule_data(const char* room_id, const struct tm* date, lesson_t** lessons, int* lesson_count)
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "Failed to initialize curl\n");
        return -1;
    }

    char url[256];
    snprintf(url, sizeof(url), "https://mapapi.susu.ru/integration/map/Schedule/roomId/%s/date/%02d.%02d.%04d",
        room_id, date->tm_mday, date->tm_mon + 1, date->tm_year + 1900);

    //printf("Fetching URL: %s\n", url);

    char* response = malloc(1);
    if (!response)
    {
        fprintf(stderr, "Failed to allocate response buffer\n");
        curl_easy_cleanup(curl);
        return -1;
    }
    response[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
        free(response);
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_cleanup(curl);

    // Parsing JSON
    cJSON* json = cJSON_Parse(response);
    if (!json)
    {
        fprintf(stderr, "Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
        free(response);
        return -1;
    }

    // Check that this is an array
    if (!cJSON_IsArray(json))
    {
        fprintf(stderr, "JSON response is not an array\n");
        cJSON_Delete(json);
        free(response);
        return -1;
    }

    // Initialize lessons and lesson_count
    *lessons = NULL;
    *lesson_count = 0;

    // Parse JSON array
    int array_size = cJSON_GetArraySize(json);
    for (int i = 0; i < array_size; i++)
    {
        cJSON* item = cJSON_GetArrayItem(json, i);
        cJSON* type = cJSON_GetObjectItem(item, "Type");
        cJSON* groups = cJSON_GetObjectItem(item, "Groups");
        cJSON* teacher = cJSON_GetObjectItem(item, "Teacher");

        // Check if Groups and Teacher are empty
        bool groups_empty = !groups || cJSON_IsNull(groups) || (cJSON_IsString(groups) && strlen(groups->valuestring) == 0);
        bool teacher_empty = !teacher || cJSON_IsNull(teacher) || (cJSON_IsString(teacher) && strlen(teacher->valuestring) == 0);

        // Skip if Type is null AND at least one of Groups or Teacher is empty
        // OR if both Groups and Teacher are empty
        if ((type && cJSON_IsNull(type) && (groups_empty || teacher_empty)) || (groups_empty && teacher_empty))
        {
            continue;
        }

        // Allocate memory for one more lesson
        lesson_t* tmp_lessons = realloc(*lessons, (*lesson_count + 1) * sizeof(lesson_t));
        if (!tmp_lessons)
        {
            fprintf(stderr, "Failed to allocate memory for lesson %d\n", i);
            free(*lessons);
            cJSON_Delete(json);
            free(response);
            return -1;
        }
        *lessons = tmp_lessons;
        lesson_t* lesson = &(*lessons)[*lesson_count];

        // Initialize default values
        lesson->subject = NULL;
        lesson->teacher = NULL;
        lesson->type = NULL;
        lesson->groups = NULL;
        lesson->color = 0xCCCCCC;
        lesson->start_hour = 0;
        lesson->start_minute = 0;
        lesson->end_hour = 0;
        lesson->end_minute = 0;

        cJSON* period = cJSON_GetObjectItem(item, "Period");
        cJSON* subject = cJSON_GetObjectItem(item, "Subject");

        // Validate JSON fields
        if (!period || !cJSON_IsString(period) ||
            !subject || !cJSON_IsString(subject) ||
            !groups || !cJSON_IsString(groups) ||
            !teacher || !cJSON_IsString(teacher))
        {
            fprintf(stderr, "Invalid JSON structure for lesson %d\n", i);
            lesson->subject = strdup("");
            lesson->teacher = strdup("");
            lesson->type = strdup("");
            lesson->groups = strdup("");
            (*lesson_count)++;
            continue;
        }

        // Time parsing ("HH:MM:SS-HH:MM:SS")
        int start_hour, start_minute, end_hour, end_minute;
        int parsed = sscanf(period->valuestring, "%02d:%02d:%*d-%02d:%02d:%*d",
            &start_hour, &start_minute, &end_hour, &end_minute);
        if (parsed != 4)
        {
            fprintf(stderr, "Failed to parse period string for lesson %d: '%s'\n", i, period->valuestring);
            lesson->subject = strdup("");
            lesson->teacher = strdup("");
            lesson->type = strdup("");
            lesson->groups = strdup("");
            (*lesson_count)++;
            continue;
        }

        lesson->start_hour = start_hour;
        lesson->start_minute = start_minute;
        lesson->end_hour = end_hour;
        lesson->end_minute = end_minute;

        lesson->subject = strdup(subject->valuestring);
        lesson->teacher = strdup(teacher->valuestring);
        lesson->groups = strdup(groups->valuestring);

        // Check for memory allocation failure
        if (!lesson->subject || !lesson->teacher || !lesson->groups)
        {
            fprintf(stderr, "Memory allocation failed for lesson %d strings\n", i);
            free(lesson->subject);
            free(lesson->teacher);
            free(lesson->groups);
            free(lesson->type);
            free(*lessons);
            cJSON_Delete(json);
            free(response);
            return -1;
        }

        if (!type || cJSON_IsNull(type))
        {
            for (size_t j = 0; type_colors[j].api_type != NULL; j++)
            {
                if (strcmp(type_colors[j].api_type, "-") == 0)
                {
                    lesson->type = strdup(type_colors[j].display_type);
                    lesson->color = type_colors[j].color;
                    break;
                }
            }
        }
        else if (cJSON_IsString(type))
        {
            lesson->type = strdup(type->valuestring);
            for (size_t j = 0; type_colors[j].api_type != NULL; j++)
            {
                if (strcmp(type->valuestring, type_colors[j].api_type) == 0)
                {
                    free(lesson->type);
                    lesson->type = strdup(type_colors[j].display_type);
                    lesson->color = type_colors[j].color;
                    break;
                }
            }
        }
        else
        {
            lesson->type = strdup("");
        }

        // Check for type memory allocation failure
        if (!lesson->type)
        {
            fprintf(stderr, "Memory allocation failed for lesson %d type\n", i);
            free(lesson->subject);
            free(lesson->teacher);
            free(lesson->groups);
            free(lesson->type);
            free(*lessons);
            cJSON_Delete(json);
            free(response);
            return -1;
        }

        (*lesson_count)++;
    }

    cJSON_Delete(json);
    free(response);
    return 0;
}
