#pragma once

#include "esp_err.h"
#include "audio_event_iface.h"

typedef enum {
    RECODE_TASK_IDLE,
    RECODE_TASK_RUNNING,
    RECODE_TASK_PAUSE,
    RECODE_TASK_RESUME,
    RECODE_TASK_STOP,
} recode_task_state_t;

esp_err_t loadRecodePipeline(audio_event_iface_handle_t *evt);
esp_err_t unloadRecodePipeline();

void startRecord();
void pauseRecord();
void stopRecord();

