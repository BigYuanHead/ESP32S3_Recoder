#include "playPipeline.h"
#include "systemWise.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"

static const char *logTAG = "RECODE_TASK";

#include "sdkconfig.h"
#include "board.h"
#include "esp_peripherals.h"
#include "fatfs_stream.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "audio_sys.h"

// aduio encode type
#include "i2s_stream.h"
#include "wav_decoder.h"
#include "opus_decoder.h"
#include "aac_decoder.h"

static const char *logTAG = "PLAY_PIPELINE";

audio_pipeline_handle_t pipeline;
audio_element_handle_t i2s_stream_writer, wav_decoder, fatfs_stream_reader, rsp_handle;
audio_event_iface_handle_t evt_pipeline = NULL;

esp_err_t loadPlayPipeline(audio_event_iface_handle_t *evt){
    esp_log_level_set(logTAG, ESP_LOG_INFO);

    ESP_LOGI(logTAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(logTAG, "[4.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(logTAG, "[4.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    i2s_stream_set_clk(i2s_stream_writer, 48000, 16, 2);

    ESP_LOGI(logTAG, "[4.2] Create wav decoder to decode wav file");
    wav_decoder_cfg_t wav_cfg = DEFAULT_WAV_DECODER_CONFIG();
    wav_decoder = wav_decoder_init(&wav_cfg);

    /* ZL38063 audio chip on board of ESP32-LyraTD-MSC does not support 44.1 kHz sampling frequency,
       so resample filter has been added to convert audio data to other rates accepted by the chip.
       You can resample the data to 16 kHz or 48 kHz.
    */
    ESP_LOGI(logTAG, "[4.3] Create resample filter");
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_handle = rsp_filter_init(&rsp_cfg);

    ESP_LOGI(logTAG, "[4.4] Create fatfs stream to read data from sdcard");
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER;
    fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
    // audio_element_set_uri(fatfs_stream_reader, url);

    ESP_LOGI(logTAG, "[4.5] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
    audio_pipeline_register(pipeline, wav_decoder, "wav");
    audio_pipeline_register(pipeline, rsp_handle, "filter");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(logTAG, "[4.6] Link it together [sdcard]-->fatfs_stream-->wav_decoder-->resample-->i2s_stream-->[codec_chip]");
    const char *link_tag[4] = {"file", "wav", "filter", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 4);

    ESP_LOGI(logTAG, "[5.0] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt_pipeline = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(logTAG, "[5.1] Listen for all pipeline events");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGW(logTAG, "[ 6 ] Press the keys to control music player:");
    ESP_LOGW(logTAG, "      [Play] to start, pause and resume, [Set] next song.");
    ESP_LOGW(logTAG, "      [Vol-] or [Vol+] to adjust volume.");

}

void _playPipelineBG() {
    mem_assert(evt_pipeline);
    mem_assert(wav_decoder);

    while (1) {
        /* Handle event interface messages from pipeline
           to set music info and to advance to the next song
        */
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt_pipeline, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(logTAG, "[ PLAY PIPLINE ] < BG > Event interface error : %d", ret);
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) {
            // Set music info for a new song to be played
            if (msg.source == (void *) wav_decoder
                && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
                audio_element_info_t music_info = {0};
                audio_element_getinfo(wav_decoder, &music_info);
                ESP_LOGI(logTAG, "[ PLAY PIPLINE ] < BG > Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                         music_info.sample_rates, music_info.bits, music_info.channels);
                audio_element_setinfo(i2s_stream_writer, &music_info);
                rsp_filter_set_src_info(rsp_handle, music_info.sample_rates, music_info.channels);
                continue;
            }
            // Advance to the next song when previous finishes
            if (msg.source == (void *) i2s_stream_writer
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                if (el_state == AEL_STATE_FINISHED) {
                    ESP_LOGI(logTAG, "[ PLAY PIPLINE  ] < BG > Finished");
                    // sdcard_list_next(sdcard_list_handle, 1, &url);
                    // ESP_LOGW(logTAG, "URL: %s", url);
                    // /* In previous versions, audio_pipeline_terminal() was called here. It will close all the element task and when we use
                    //  * the pipeline next time, all the tasks should be restarted again. It wastes too much time when we switch to another music.
                    //  * So we use another method to achieve this as below.
                    //  */
                    // audio_element_set_uri(fatfs_stream_reader, url);
                    // audio_pipeline_reset_ringbuffer(pipeline);
                    // audio_pipeline_reset_elements(pipeline);
                    // audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                    // audio_pipeline_run(pipeline);
                }
                continue;
            }
        }
    }
}

void startPlay( ) {
    mem_assert(i2s_stream_writer)

    audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
    if (el_state == AEL_STATE_INIT){
        ESP_LOGI(logTAG, "[ START PLAY ] Starting audio pipeline");
        audio_pipeline_run(pipeline);
    } else {

    }
}            

void pausePlay() {

    audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
    if (el_state == AEL_STATE_RUNNING){
        ESP_LOGI(logTAG, "[ PAUSE PLAY ] Starting audio pipeline");
        audio_pipeline_pause(pipeline);
    } else {

    }
}

void resumePlay() {
    audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
    if (el_state == AEL_STATE_PAUSED){
        ESP_LOGI(logTAG, "[ RESUME PLAY ] Starting audio pipeline");
        audio_pipeline_resume(pipeline);
    } else {

    }
}

void stopPlay() {
    ESP_LOGI(logTAG, "[ 7 ] Stop audio_pipeline");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    
}

void setMusic( char *url ) {
    ESP_LOGI(logTAG, "[ * ] Stopped, advancing to the next song");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);
    // sdcard_list_next(sdcard_list_handle, 1, &url);
    ESP_LOGW(logTAG, "URL: %s", url);
    audio_element_set_uri(fatfs_stream_reader, url);
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);
    audio_pipeline_run(pipeline);
}

void volumeUp() {
    ESP_LOGI(logTAG, "[ * ] [Vol+] input key event");
    player_volume += 10;
    if (player_volume > 100) {
        player_volume = 100;
    }
    audio_hal_set_volume(board_handle->audio_hal, player_volume);
    ESP_LOGI(logTAG, "[ * ] Volume set to %d %%", player_volume);
            
}

void volumDown() {
    ESP_LOGI(logTAG, "[ * ] [Vol-] input key event");
    player_volume -= 10;
    if (player_volume < 0) {
        player_volume = 0;
    }
    audio_hal_set_volume(board_handle->audio_hal, player_volume);
    ESP_LOGI(logTAG, "[ * ] Volume set to %d %%", player_volume);
}

esp_err_t unloadPlayPipeline(){
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, wav_decoder);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);
    audio_pipeline_unregister(pipeline, rsp_handle);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Stop all peripherals before removing the listener */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    sdcard_list_destroy(sdcard_list_handle);
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(mp3_decoder);
    audio_element_deinit(rsp_handle);
    periph_service_destroy(input_ser);
    esp_periph_set_destroy(set);
}