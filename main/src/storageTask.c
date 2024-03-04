// #include "storageTask.h"
// #include "esp_log.h"

// #include "board.h"
// #include "esp_peripherals.h"
// #include "periph_sdcard.h"
// #include "fatfs_stream.h"

// static const char *logTAG = "STORAGE_TASK";
// audio_element_handle_t fatfs_stream_writer;

// void loadStorageTask(){

//     ESP_LOGI(logTAG, " ++ Loading Storage TASK ++ ");
    
//     ESP_LOGI(logTAG, "[ 1 ] Mount sdcard");
//     esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
//     esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

//     // Initialize SD Card peripheral
//     audio_board_sdcard_init(set, SD_MODE_1_LINE);
    
//     ESP_LOGI(logTAG, "[ 2 ] Create fatfs stream to write data to sdcard");
//     fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
//     fatfs_cfg.type = AUDIO_STREAM_WRITER;
//     fatfs_stream_writer = fatfs_stream_init(&fatfs_cfg);

// }