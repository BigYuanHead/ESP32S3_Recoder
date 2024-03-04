#ifndef _SYSTEMINFO_H_
#define _SYSTEMINFO_H_


// #define BUTTON_VOLUP_ID             0
// #define BUTTON_VOLDOWN_ID           1
// #define BUTTON_SET_ID               2
// #define BUTTON_PLAY_ID              3
// #define BUTTON_MODE_ID              4
// #define BUTTON_REC_ID               5

typedef enum {
    TOP_MENU_MODE_KEY = 4,
} mapKey_topMenu_type_t;

typedef enum {
    RECODE_MENU_MODE_KEY = 4,
    RECODE_MENU_START_KEY = 5,
    RECODE_MENU_PAUSE_KEY = 3,
    RECODE_MENU_STOP_KEY = 0,
    RECODE_MENU_MARK_KEY = 2,
} mapKey_recodeMenu_type_t;

typedef enum {
    TOP_MENU_MODE_RECODE,
    TOP_MENU_MODE_PLAY,
} top_menu_type_t;

typedef enum {
    RECODE_MENU_MODE, //IDLE
    RECODE_MENU_START,
    RECODE_MENU_PAUSE,
    RECODE_MENU_STOP,
    RECODE_MENU_MARK,
} recode_menu_type_t;

typedef struct missionState
{
    top_menu_type_t currentTopMenu;
    recode_menu_type_t currentRecodeMenu;
} missionState_s ;

#define MISSION_STATE_DEFAULT() {             \
    .currentTopMenu = TOP_MENU_MODE_RECODE,   \
    .currentRecodeMenu = RECODE_MENU_MODE,     \
}


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