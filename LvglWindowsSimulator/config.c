#include "config.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_room_id_from_config(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        fprintf(stderr, "Failed to open config file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length <= 0)
    {
        fprintf(stderr, "Config file is empty or error reading length\n");
        fclose(file);
        return NULL;
    }

    char* buffer = (char*)malloc((size_t)length + 1);
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate memory for config buffer\n");
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, (size_t)length, file);
    fclose(file);

    if (read_bytes != (size_t)length)
    {
        fprintf(stderr, "Failed to read config file contents\n");
        free(buffer);
        return NULL;
    }
    buffer[length] = '\0';

    cJSON* json = cJSON_Parse(buffer);
    free(buffer);

    if (!json)
    {
        fprintf(stderr, "Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
        return NULL;
    }

    cJSON* room_id = cJSON_GetObjectItem(json, "roomId");
    if (!cJSON_IsString(room_id))
    {
        fprintf(stderr, "roomId is not a string\n");
        cJSON_Delete(json);
        return NULL;
    }

    char* result = strdup(room_id->valuestring);
    cJSON_Delete(json);
    return result;
}
