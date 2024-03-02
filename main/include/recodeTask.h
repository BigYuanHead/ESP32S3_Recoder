#ifndef _RECODE_TASK_H_
#define _RECODE_TASK_H_

typedef enum {
    RECODE_TASK_IDLE,
    RECODE_TASK_RUNNING,
    RECODE_TASK_PAUSE,
    RECODE_TASK_RESUME,
    RECODE_TASK_STOP,
} recode_task_state_t;

void loadRecodeTask();
void unloadRecodeTask();

void startRecord();
void pauseRecord();
void stopRecord();


#endif _RECODE_TASK_H_