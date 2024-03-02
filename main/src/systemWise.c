#include "systemWise.h"
#include "audio_event_iface.h"


static const char *logTAG = "systemWise";

audio_event_iface_handle_t evt_input = NULL;
audio_event_iface_handle_t evt_display = NULL;
audio_event_iface_handle_t evt_media = NULL;

sys_status_type_t sys_current_state = SYS_STATU_RECORD;

// internal declare
esp_err_t buttonSentEvents(int cmd, void *data, int data_len);
esp_err_t displaySentEvents(int cmd, void *data, int data_len);
esp_err_t mediaSentEvents(int cmd, void *data, int data_len);

void initSystemWise() {

    esp_log_level_set("*", ESP_LOG_WARN);
    // TODO: set time

    // systemWise Event
}

void _stateSwitcher() {

    while(1){
        // 1. rec from input task
        audio_event_iface_msg_t but_msg;
        esp_err_t ret = audio_event_iface_listen(evt_input, &but_msg, portMAX_DELAY);
        if (ret != ESP_OK) 
            continue;
        
        if ( sys_current_state == SYS_STATU_RECORD ){
            if( but_msg.source_type == INPUT_START ) {
                mediaSentEvents(MEDIA_RECORD_START, NULL, 0);
            } else if( but_msg.source_type == INPUT_PAUSE ) {
                mediaSentEvents(MEDIA_RECORD_PAUSE, NULL, 0);
            } else if( but_msg.source_type == INPUT_STOP ) {
                mediaSentEvents(MEDIA_RECORD_STOP, NULL, 0);
            } else if( but_msg.source_type == INPUT_MARK ) {
                mediaSentEvents(MEDIA_RECORD_MARK, NULL, 0);
            } else if( but_msg.source_type == INPUT_MODE_RRESS_SHORT ) {
                displaySentEvents(DISPLAY_MEDIA_PLAY, NULL, 0);
            } else{
                ESP_LOGE(logTAG, "Should NOT be here");
            }
        }
        else if ( sys_current_state == SYS_STATU_DISPLAY ){
            if( but_msg.source_type == INPUT_START ) {
                mediaSentEvents(MEDIA_RECORD_START, NULL, 0);
            }
            else if( but_msg.source_type == INPUT_PAUSE ) {
                mediaSentEvents(MEDIA_RECORD_PAUSE, NULL, 0);
            }
            else if( but_msg.source_type == INPUT_STOP ) {
                mediaSentEvents(MEDIA_RECORD_STOP, NULL, 0);
            }
            else if( but_msg.source_type == INPUT_MARK ) {
                mediaSentEvents(MEDIA_RECORD_MARK, NULL, 0);
            }
            else{
                ESP_LOGE(logTAG, "Should NOT be here");
            }
        }
        else {
            ESP_LOGE(logTAG, "Should NOT be here");
        }
        
    }
}



void setEvents() {
    ESP_LOGI(logTAG, "[ 1 ] Set up events");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();

    ESP_LOGI(logTAG, "[ 1.1 ] Set up inputTask events");
    evt_input = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(logTAG, "[ 1.2 ] Set up displayTask events");
    evt_display = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(logTAG, "[ 1.3 ] Set up mediaTask events");
    evt_media = audio_event_iface_init(&evt_cfg);
}

esp_err_t buttonSentEvents(int cmd, void *data, int data_len)
{
    if (evt_input == NULL) {
        return ESP_FAIL;
    }
    audio_event_iface_msg_t msg;
    msg.cmd = cmd;
    msg.source = NULL;
    msg.source_type = TASK_INPUT;
    msg.data = (void *)data;
    msg.data_len = data_len;
    return audio_event_iface_cmd(evt_input, &msg);
}


esp_err_t displaySentEvents(int cmd, void *data, int data_len) {
    if (evt_input == NULL) {
        return ESP_FAIL;
    }
    audio_event_iface_msg_t msg;
    msg.cmd = cmd;
    msg.source = NULL;
    msg.source_type = TASK_INPUT;
    msg.data = (void *)data;
    msg.data_len = data_len;
    return audio_event_iface_cmd(evt_input, &msg);
}

esp_err_t mediaSentEvents(int cmd, void *data, int data_len){
    if (evt_input == NULL) {
        return ESP_FAIL;
    }
    audio_event_iface_msg_t msg;
    msg.cmd = cmd;
    msg.source = NULL;
    msg.source_type = TASK_INPUT;
    msg.data = (void *)data;
    msg.data_len = data_len;
    return audio_event_iface_cmd(evt_input, &msg);
}





