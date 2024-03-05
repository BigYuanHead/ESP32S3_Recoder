#include "systemWise.h"

#include "audio_event_iface.h"
#include "board.h"
#include "esp_peripherals.h"
#include "periph_button.h"
#include "periph_sdcard.h"

#include "recodePipeline.h"

static const char *logTAG = "systemWise";

#define RECODE_TASK_STACK_SIZE ( 4*1024 )
#define RECODE_TASK_PRIORITY 6
#define MISSION_TASK_STACK_SIZE ( 10*1024 )
#define MISSION_TASK_PRIORITY 6
TaskHandle_t recodeTask_handle = NULL;

sys_status_type_t sys_current_state = SYS_STATU_RECORD;

// periph_set_handle 
esp_periph_set_handle_t periph_set = NULL;
audio_event_iface_handle_t evt_periph = NULL;
audio_event_iface_handle_t evt_pipeline = NULL;

// internal functions
uint8_t _myKeyListener(uint16_t waitTime);
void _recodeMission(mapKey_recodeMenu_type_t type);
void _recordBG();
void _missionManager();

void initSystemWise() {

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("SDCARD", ESP_LOG_DEBUG);
    esp_log_level_set(logTAG, ESP_LOG_DEBUG);

    ESP_LOGI(logTAG, "[ 1 ] Initialize all peripherals");

    ESP_LOGI(logTAG, "[ 1.1 ] Initialize peripherals handler");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    periph_set = esp_periph_set_init(&periph_cfg);

    ESP_LOGI(logTAG, "[ 1.2 ] Initialize keys on board");
    if(audio_board_key_init(periph_set)!= ESP_OK ){
        ESP_LOGE(logTAG, " init Key fail ");
        return;
    }

    ESP_LOGI(logTAG, "[ 1.3 ] Mount sdcard");
    // Initialize SD Card peripheral
    if(audio_board_sdcard_init(periph_set, SD_MODE_1_LINE) != ESP_OK ){
        ESP_LOGE(logTAG, " Mount SDcard fail ");
        return;
    }

    ESP_LOGI(logTAG, "[ 2 ] Listen event from peripherals");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt_periph = audio_event_iface_init(&evt_cfg);
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(periph_set), evt_periph);

    // TODO: set time
    evt_pipeline = audio_event_iface_init(&evt_cfg);
    xTaskCreate(_missionManager, "missionManager", MISSION_TASK_STACK_SIZE, (void*)NULL, MISSION_TASK_PRIORITY, NULL);
    // systemWise Event
}

void deinitSystemWise(){
    mem_assert(periph_set);
    esp_periph_set_destroy(periph_set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(periph_set), evt_periph);
}

static missionState_s currentMissionState = MISSION_STATE_DEFAULT();

void _missionManager() {
    mem_assert(evt_pipeline)
    // initially load recode mission
    loadRecodePipeline(&evt_pipeline);

    while(1){
        //TODO: back to top menu
        uint8_t ret = _myKeyListener(1000);
        if(currentMissionState.currentTopMenu == TOP_MENU_MODE_RECODE){
            _recodeMission(ret);
        }else if (currentMissionState.currentTopMenu == TOP_MENU_MODE_PLAY){
            ESP_LOGI(logTAG, "[ MISSION ] & PLAY & NOT EXIST YET");
        }
        
    }
}


void _recodeMission(mapKey_recodeMenu_type_t type) {
    // ESP_LOGW(logTAG, "[ 5 ] Tap touch buttons to control music player:");
    // ESP_LOGW(logTAG, "      [Play] to start, pause and resume, [Set] to stop.");
    // ESP_LOGW(logTAG, "      [Vol-] or [Vol+] to adjust volume.");

    switch( type ) {
        case RECODE_MENU_MODE_KEY:
            ESP_LOGI(logTAG, "[ RECODE ] < ESC >");
            // TODO: Return back to Main menu
            break;
        case RECODE_MENU_START_KEY:
            ESP_LOGI(logTAG, "[ RECODE ] < START >");
            startRecord();
            ESP_LOGI(logTAG, "[ 7 ] Set up BG monitor");
            xTaskCreate(_recordBG, "recodeTaskBG", RECODE_TASK_STACK_SIZE, (void*)NULL, RECODE_TASK_PRIORITY, recodeTask_handle);
            currentMissionState.currentRecodeMenu = RECODE_MENU_START;
            break;
        case RECODE_MENU_PAUSE_KEY:
            ESP_LOGI(logTAG, "[ RECODE ] < PAUSE >");
            // TODO: Return back to Main menu
            pauseRecord();
            break;
        case RECODE_MENU_STOP_KEY:
            ESP_LOGI(logTAG, "[ RECODE ] < STOP >");
            // TODO: Return back to Main menu
            stopRecord();
            break;
        case RECODE_MENU_MARK_KEY:
            ESP_LOGI(logTAG, "[ RECODE ] < MARK >");
            // TODO: Return back to Main menu
            break;
        default:{
            ESP_LOGD(logTAG, "[ RECODE ] < NONE >");
            break;
        }
    }
}



uint8_t _myKeyListener(uint16_t waitTime) {
    mem_assert(evt_periph);
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(evt_periph, &msg, waitTime);
    if (ret != ESP_OK) {
        return 99;
    }
    else{
        if ((msg.source_type == PERIPH_ID_ADC_BTN ) && ( msg.cmd == PERIPH_BUTTON_PRESSED )) {
            if ((int) msg.data == get_input_volup_id()) {
                return 0;
            } else if ((int) msg.data == get_input_voldown_id()) {
                return 1;
            } else if ((int) msg.data == get_input_set_id()) {
                return 2;
            } else if ((int) msg.data == get_input_play_id()) {
                return 3;
            } else if ((int) msg.data == get_input_mode_id()) {
                return 4;
            } else if ((int) msg.data == get_input_rec_id()) {
                return 5;
            } else {
                return 98;
            }
        }
    }
    return 99;
}




/* Background of recode pipline */
void _recordBG() {
    mem_assert(evt_pipeline);

    int second_recorded = 0;
    while (1) {
        audio_event_iface_msg_t pipeline_msg;

        if (audio_event_iface_listen(evt_pipeline, &pipeline_msg, 500 / portTICK_RATE_MS) != ESP_OK) {
            if (currentMissionState.currentRecodeMenu == RECODE_MENU_START){
                // audio_element_info_t music_info = {0};
                // audio_element_getinfo(i2s_stream_reader, &music_info);
                // ESP_LOGI(logTAG, "[ RECODE_BG *** ] Recording ... %d", music_info.duration);
                ESP_LOGI(logTAG, "[ RECODE_BG *** ] Recording ... %d", second_recorded);
                second_recorded ++;
                // TODO: update buffer to display

                continue;
            }
            
    //     } else {
    //         if (currentMissionState.currentRecodeMenu == RECODE_MENU_STOP) {
    //                 /* Stop when the last pipeline element (fatfs_stream_writer in this case) receives stop event */
    //                 if (pipeline_msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && pipeline_msg.source == (void *) fatfs_stream_writer
    //                     && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
    //                     && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)
    //                         || ((int)msg.data == AEL_STATUS_ERROR_OPEN))) {
    //                     ESP_LOGW(logTAG, "[ RECODE_BG *** ] Recording stopped");
    //                     break;
    //                 }
    //             }
            }    
    }
        
}