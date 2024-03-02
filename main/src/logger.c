#include "logger.h"

static const char *logTAG = "Logger";

void initLogger(){
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(logTAG, ESP_LOG_INFO);
}