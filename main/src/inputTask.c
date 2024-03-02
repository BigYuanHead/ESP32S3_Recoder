
#include "inputTask.h"
#include "esp_log.h"

#include "board.h"
#include "esp_peripherals.h"
#include "periph_button.h"

static const char *logTAG = "inputTask";

void _InputTaskBG();
#define INPUT_TASK_STACK_SIZE  (2048)
#define INPUT_TASK_PRIORITY    2
TaskHandle_t *inputTask_handle = NULL;

audio_event_iface_handle_t evt_key = NULL;

void loadInputTask() {
    esp_log_level_set(logTAG, ESP_LOG_DEBUG);

    ESP_LOGI(logTAG, "[ 1 ] Initialize peripherals");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    ESP_LOGI(logTAG, "[ 2 ] Initialize keys on board");
    audio_board_key_init(set);

    ESP_LOGI(logTAG, "[ 3 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt_key = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(logTAG, "[ 4 ] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt_key);

    ESP_LOGW(logTAG, "[ 5 ] Tap touch buttons to control music player:");
    ESP_LOGW(logTAG, "      [Play] to start, pause and resume, [Set] to stop.");
    ESP_LOGW(logTAG, "      [Vol-] or [Vol+] to adjust volume.");

    ESP_LOGI(logTAG, "[ 6 ] Set up BG monitor");
    xTaskCreate(_InputTaskBG, "inputTaskBG", INPUT_TASK_STACK_SIZE, (void*)NULL, INPUT_TASK_PRIORITY, inputTask_handle);

}

void _InputTaskBG() {
    mem_assert(evt_key)

    while (1) {

        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt_key, &msg, 1000);
        if (ret != ESP_OK) {
            continue;
        }
        else{
            ESP_LOGI(logTAG, "msg type %d", msg.source_type);
            ESP_LOGI(logTAG, "msg cmd %d", msg.cmd);

            if ((msg.source_type == PERIPH_ID_ADC_BTN ) && ( msg.cmd == PERIPH_BUTTON_PRESSED )) {
                if ((int) msg.data == get_input_play_id()) {
                    ESP_LOGI(logTAG, "[ * ] [Play] tap event");
                    // TODO
                    
                } else if ((int) msg.data == get_input_set_id()) {
                    ESP_LOGI(logTAG, "[ * ] [Set] tap event");
                    // TODO
                } else if ((int) msg.data == get_input_mode_id()) {
                    ESP_LOGI(logTAG, "[ * ] [Mode] tap event");
                    // TODO
                } else if ((int) msg.data == get_input_rec_id()) {
                    ESP_LOGI(logTAG, "[ * ] [Rec] tap event");
                    // TODO
                } else if ((int) msg.data == get_input_volup_id()) {
                    ESP_LOGI(logTAG, "[ * ] [Vol+] tap event");
                    // TODO
                } else if ((int) msg.data == get_input_voldown_id()) {
                    ESP_LOGI(logTAG, "[ * ] [Vol-] tap event");
                    // TODO
                }
                else{
                    ESP_LOGI(logTAG, "[ * ] [None] tap event");
                }
            }
        }
    }

}