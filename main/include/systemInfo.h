#ifndef _SYSTEMINFO_H_
#define _SYSTEMINFO_H_

typedef enum {
    SYS_STATU_IDLE,
    SYS_STATU_RECORD,
    SYS_STATU_DISPLAY,
} sys_status_type_t;


/** ++++++ ALL TASKS +++++++ **/

typedef enum {
    TASK_INPUT,
    TASK_DISPLAY,
    TASK_MEDIA_PLAY,
    TASK_MEDIA_RECORD,
} sys_task_id_t;

struct sys_Task {
    char                       *tag;
    bool                        disabled;
    sys_task_id_t             periph_id;
    // esp_periph_func             init;
    // esp_periph_run_func         run;
    // esp_periph_func             destroy;
    // esp_periph_state_t          state;
    // void                       *source;
    // void                       *periph_data;
};


/** ++++++ ALL TASKS STATUS +++++++ **/

typedef enum  {
    INPUT_UNKNOWN,              /*!< No event */
    INPUT_MODE_RRESS_SHORT,
    INPUT_MODE_RRESS_LONG,
    INPUT_MARK,   /*!< Markdown file */
    INPUT_START,              /*!<  */
    INPUT_PAUSE,            /*!<  */
    INPUT_STOP,          /*!<  */
    INPUT_VOL_UP,
    INPUT_VOL_DOWN,
    INPUT_MODE_SWITCH,
    INPUT_PREVIOUS,
    INPUT_NEXT,
} sys_task_input_event_id_t;

typedef enum  {
    DISPLAY_UNKNOWN,              /*!< No event */
    DISPLAY_MEDIA_RECORD,   /*!< Markdown file */
    DISPLAY_MEDIA_PLAY,              /*!<  */
} sys_task_display_event_id_t;

typedef enum  {
    MEDIA_RECORD_UNKNOWN,              /*!< No event */
    MEDIA_RECORD_MARK,
    MEDIA_RECORD_START,
    MEDIA_RECORD_PAUSE,
    MEDIA_RECORD_STOP,
    MEDIA_PLAY_UNKNOW,
    MEDIA_PLAY_NEXT,
    MEDIA_PLAY_PREVIOUS,
    MEDIA_PLAY_VOL_UP,
    MEDIA_PLAY_VOL_DOWN,
    MEDIA_PLAY_START,
    MEDIA_PLAY_PAUSE,
    MEDIA_PLAY_STOP,
} sys_task_media_event_id_t;


#endif _SYSTEMINFO_H_