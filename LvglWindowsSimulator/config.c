#include "config.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Config read_config(const char* filename)
{
    Config config = { .roomId = NULL, .isDarkTheme = false, .inactiveDurationMs = 60000 }; // Default values

    // Read the file
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Failed to open config file: %s\n", filename);
        return config;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read file content
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer)
    {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return config;
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    fclose(file);

    // Parse JSON
    cJSON* json = cJSON_Parse(buffer);
    free(buffer);
    if (!json)
    {
        fprintf(stderr, "Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
        return config;
    }

    // Read roomId
    cJSON* room_id_item = cJSON_GetObjectItem(json, "roomId");
    if (cJSON_IsString(room_id_item) && room_id_item->valuestring != NULL)
    {
        config.roomId = strdup(room_id_item->valuestring);
        if (!config.roomId)
        {
            fprintf(stderr, "Failed to allocate memory for roomId\n");
        }
    }
    else
    {
        fprintf(stderr, "roomId not found or not a string in config\n");
    }

    // Read isDarkTheme
    cJSON* theme_item = cJSON_GetObjectItem(json, "isDarkTheme");
    if (cJSON_IsBool(theme_item))
    {
        config.isDarkTheme = cJSON_IsTrue(theme_item);
    }
    else
    {
        fprintf(stderr, "isDarkTheme not found or not a boolean in config\n");
    }

    // Read inactiveDurationMs
    cJSON* inactive_duration_item = cJSON_GetObjectItem(json, "inactiveDurationMs");
    if (cJSON_IsNumber(inactive_duration_item))
    {
        config.inactiveDurationMs = (uint32_t)inactive_duration_item->valuedouble;
        if (config.inactiveDurationMs == 0)
        {
            fprintf(stderr, "inactiveDurationMs is invalid, using default: 60000\n");
            config.inactiveDurationMs = 60000;
        }
    }
    else
    {
        fprintf(stderr, "inactiveDurationMs not found or not a number in config\n");
    }

    cJSON_Delete(json);
    return config;
}
