#include "recodeTask.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

static const char *logTAG = "RECODE_TASK";

#include "sdkconfig.h"
#include "board.h"
#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "fatfs_stream.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "audio_sys.h"

// aduio encode type
#include "i2s_stream.h"
#include "wav_encoder.h"
#include "opus_encoder.h"
#include "aac_encoder.h"
#include "amrwb_encoder.h"
#include "amrnb_encoder.h"

void _recordBG();
#define RECODE_TASK_STACK_SIZE  (2048 + 1024)
#define RECODE_TASK_PRIORITY    3
TaskHandle_t *recodeTask_handle = NULL;


// WAV
#define WAV_SAMPLE_RATE 16000
// （CONFIG_CHOICE_OPUS_ENCODER)
#define OPUS_SAMPLE_RATE         16000
#define OPUS_CHANNEL             1
#define OPUS_BIT_RATE            64000
#define OPUS_COMPLEXITY          10
// (CONFIG_CHOICE_AAC_ENCODER)
#define AAC_SAMPLE_RATE         16000
#define AAC_CHANNEL             2
#define AAC_BIT_RATE            80000

typedef enum {
    AUDIO_DATA_FORMNAT_ONLY_RIGHT,
    AUDIO_DATA_FORMNAT_ONLY_LEFT,
    AUDIO_DATA_FORMNAT_RIGHT_LEFT,
} audio_channel_format_t;

typedef enum {
    WAV,
    OPUS,
    AAC,
    AMRNB,
    AMRWB,
} audio_encode_type_t;

static audio_encode_type_t audioEncodeType = WAV;
static recode_task_state_t recodeTaskCurrentState = RECODE_TASK_IDLE;

static esp_err_t audio_data_format_set(i2s_stream_cfg_t *i2s_cfg, audio_channel_format_t fmt)
{
    AUDIO_UNUSED(i2s_cfg);   // remove unused warning
    switch (fmt) {
        case AUDIO_DATA_FORMNAT_ONLY_RIGHT:
            i2s_cfg->i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
            break;
        case AUDIO_DATA_FORMNAT_ONLY_LEFT:
            i2s_cfg->i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
            break;
        case AUDIO_DATA_FORMNAT_RIGHT_LEFT:
            i2s_cfg->i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
            break;
    }
    return ESP_OK;
}

audio_pipeline_handle_t pipeline = NULL;
audio_element_handle_t i2s_stream_reader = NULL;
audio_element_handle_t audio_encoder = NULL;
audio_element_handle_t fatfs_stream_writer = NULL;
audio_event_iface_handle_t evt = NULL;
esp_periph_set_handle_t periph_set = NULL;

void loadRecodeTask(){

    ESP_LOGI(logTAG, " ++ Loading Recode TASK ++ ");

    int channel_format = AUDIO_DATA_FORMNAT_RIGHT_LEFT;
    int sample_rate = 16000;

    ESP_LOGI(logTAG, "[ ** ] Mount sdcard");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t periph_set = esp_periph_set_init(&periph_cfg);
    // Initialize SD Card peripheral
    audio_board_sdcard_init(periph_set, SD_MODE_1_LINE);

    ESP_LOGI(logTAG, "[ 1 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);
    
    ESP_LOGI(logTAG, "[ 2 ] Create audio pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(logTAG, "[ 3 ] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_READER;
    switch (audioEncodeType){
        case WAV:
            sample_rate = WAV_SAMPLE_RATE; break;
            break;
        case OPUS:
            sample_rate = OPUS_SAMPLE_RATE; break;
        case AAC:
            sample_rate = AAC_SAMPLE_RATE; break;
        case AMRNB:
            sample_rate = 8000; break;
        case AMRWB:
            sample_rate = 16000; break;
        default:
            break;
    }
    audio_data_format_set(&i2s_cfg, channel_format);
    i2s_cfg.i2s_config.sample_rate = sample_rate;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(logTAG, "[ 4 ] Create audio encoder to handle data");
    switch (audioEncodeType){
        case WAV:{
            wav_encoder_cfg_t wav_cfg = DEFAULT_WAV_ENCODER_CONFIG();
            audio_encoder = wav_encoder_init(&wav_cfg);
            break;
        }
        case OPUS: {
            opus_encoder_cfg_t opus_cfg = DEFAULT_OPUS_ENCODER_CONFIG();
            opus_cfg.sample_rate        = OPUS_SAMPLE_RATE;
            opus_cfg.channel            = OPUS_CHANNEL;
            opus_cfg.bitrate            = OPUS_BIT_RATE;
            opus_cfg.complexity         = OPUS_COMPLEXITY;
            audio_encoder = encoder_opus_init(&opus_cfg);
            break;
        }
        case AAC:{
            aac_encoder_cfg_t aac_cfg = DEFAULT_AAC_ENCODER_CONFIG();
            aac_cfg.sample_rate        = AAC_SAMPLE_RATE;
            aac_cfg.channel            = AAC_CHANNEL;
            aac_cfg.bitrate            = AAC_BIT_RATE;
            audio_encoder = aac_encoder_init(&aac_cfg);
            break;
        }
        case AMRNB:{
            amrwb_encoder_cfg_t amrwb_enc_cfg = DEFAULT_AMRWB_ENCODER_CONFIG();
            audio_encoder = amrwb_encoder_init(&amrwb_enc_cfg);
            break;
        }
        case AMRWB:{
            amrnb_encoder_cfg_t amrnb_enc_cfg = DEFAULT_AMRNB_ENCODER_CONFIG();
            audio_encoder = amrnb_encoder_init(&amrnb_enc_cfg);
            break;
        }
        default:{
            ESP_LOGE(logTAG, "No such formate");
            return;
        }
    }
    

    ESP_LOGI(logTAG, "[ 5 ] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    switch (audioEncodeType){
        case WAV:
            audio_pipeline_register(pipeline, audio_encoder, "wav"); break;
        case OPUS:
            audio_pipeline_register(pipeline, audio_encoder, "opus"); break;
        case AAC:
            audio_pipeline_register(pipeline, audio_encoder, "aac"); break;
        case AMRNB:
            audio_pipeline_register(pipeline, audio_encoder, "amr"); break;
        case AMRWB:
            audio_pipeline_register(pipeline, audio_encoder, "Wamr"); break;
        default:
            ESP_LOGE(logTAG, "No such formate");
            return;
    }
    audio_pipeline_register(pipeline, fatfs_stream_writer, "file");

    ESP_LOGI(logTAG, "[ 6 ] Link it together [codec_chip]-->i2s_stream-->audio_encoder-->fatfs_stream-->[sdcard]");
    char *link_tag[3] = {"i2s", "PEND", "file"};
    switch (audioEncodeType){
        case WAV:
            link_tag[1] = "wav"; break;
        case OPUS:
            link_tag[1] = "opus"; break;
        case AAC:
            link_tag[1] = "aac"; break;
        case AMRNB:
            link_tag[1] = "amr"; break;
        case AMRWB:
            link_tag[1] = "Wamr"; break;
        default:
            ESP_LOGE(logTAG, "No such formate");
            return;
    }
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    ESP_LOGI(logTAG, "[ 6 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);
    mem_assert(evt);

    ESP_LOGI(logTAG, "[6.1] Listening event from pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(logTAG, "[6.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(periph_set), evt);

    ESP_LOGI(logTAG, "[ 7 ] Set up BG monitor");
    xTaskCreate(_recordBG, "recodeTaskBG", RECODE_TASK_STACK_SIZE, (void*)NULL, RECODE_TASK_PRIORITY, recodeTask_handle);

}

void startRecord() {
    mem_assert(i2s_stream_reader);
    mem_assert(fatfs_stream_writer);
    mem_assert(pipeline);

    ESP_LOGI(logTAG, "[ START_REC ] Set music info to fatfs");
    audio_element_info_t music_info = {0};
    // music_info.byte_pos = 
    audio_element_getinfo(i2s_stream_reader, &music_info);
    ESP_LOGI(logTAG, "[ START_REC ] Save the recording info to the fatfs stream writer, sample_rates=%d, bits=%d, ch=%d",
                music_info.sample_rates, music_info.bits, music_info.channels);
    audio_element_setinfo(fatfs_stream_writer, &music_info);

    ESP_LOGI(logTAG, "[ START_REC ] Set up uri");
    char *fileName = "";
    switch (audioEncodeType){
        case WAV:
            fileName = "/sdcard/rec.wav"; break;
        case OPUS:
            fileName = "/sdcard/rec.opus"; break;
        case AAC:
            fileName = "/sdcard/rec.aac"; break;
        case AMRNB:
            fileName = "/sdcard/rec.amr"; break;
        case AMRWB:
            fileName = "/sdcard/rec.Wamr"; break;
        default:
            return;
    }
    audio_element_set_uri(fatfs_stream_writer, fileName);

    ESP_LOGI(logTAG, "[ START_REC ] Start audio_pipeline");
    audio_pipeline_run(pipeline);
}

/* Background of recode pipline */
void _recordBG() {
    int second_recorded = 0;
    while (1) {
        audio_event_iface_msg_t msg;
        if (audio_event_iface_listen(evt, &msg, 100 / portTICK_RATE_MS) != ESP_OK) {
            if (recodeTaskCurrentState == RECODE_TASK_RUNNING){
                audio_element_info_t music_info = {0};
                audio_element_getinfo(i2s_stream_reader, &music_info);
                ESP_LOGI(logTAG, "[ RECODE_BG *** ] Recording ... %d", music_info.duration);

                second_recorded ++;
                // TODO: update buffer to display

                continue;
            }
            
        }
        else {
            if (recodeTaskCurrentState == RECODE_TASK_STOP) {
                    /* Stop when the last pipeline element (fatfs_stream_writer in this case) receives stop event */
                    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) fatfs_stream_writer
                        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
                        && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)
                            || ((int)msg.data == AEL_STATUS_ERROR_OPEN))) {
                        ESP_LOGW(logTAG, "[ RECODE_BG *** ] Recording stopped");
                        break;
                    }
                }
            }    
        }
        
}

void pauseRecord() {
    mem_assert(pipeline);

    ESP_LOGW(logTAG, "[ PAUSE_REC ] Pause pipeline");
    esp_err_t ret = audio_pipeline_pause(pipeline);
}

void stopRecord() {
    mem_assert(pipeline);

    // 1. stop pipline    
    ESP_LOGW(logTAG, "[ STOP_REC ] Stop pipeline");
    esp_err_t ret = audio_pipeline_stop(pipeline);
    // audio_element_set_ringbuf_done(i2s_stream_reader);
    // 2. unload Task
}

void unloadRecodeTask(){
    ESP_LOGI(logTAG, " ++ Unloading Recode TASK ++ ");

    mem_assert(pipeline);
    mem_assert(i2s_stream_reader);
    mem_assert(audio_encoder);
    mem_assert(fatfs_stream_writer);
    mem_assert(evt);
    mem_assert(periph_set);
    mem_assert(recodeTask_handle);

    vTaskDelete(recodeTask_handle);

    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, audio_encoder);
    audio_pipeline_unregister(pipeline, i2s_stream_reader);
    audio_pipeline_unregister(pipeline, fatfs_stream_writer);

    /* Terminal the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Stop all periph before removing the listener */
    esp_periph_set_stop_all(periph_set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(periph_set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(fatfs_stream_writer);
    audio_element_deinit(i2s_stream_reader);
    audio_element_deinit(audio_encoder);
    esp_periph_set_destroy(periph_set);
}

