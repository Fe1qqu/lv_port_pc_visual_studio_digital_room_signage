#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

// Structure to hold configuration parameters
typedef struct
{
    char* roomId; // Room ID from config
    bool isDarkTheme; // Dark theme flag
    uint32_t inactiveDurationMs; // Inactivity duration in milliseconds
} Config;

// Function to read configuration from JSON file
Config read_config(const char* filename);

#endif
