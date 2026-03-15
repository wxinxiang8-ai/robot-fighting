#ifndef __ROBOT_BACKUP_H
#define __ROBOT_BACKUP_H

#include "main.h"

typedef enum{
    GOUP_ON,      // 在台状态
    GOUP_FALL,      // 掉台状态
} BackUpstate_t;

extern BackUpstate_t Backup_State;

#define BACKUP_SPIN_TIME_MS      500
#define BACKUP_FORWARD_TIME_MS   500
#define BACKUP_BACK_TIME_MS      500

void Backup_Init(void);
void Backup_Update(void);

#endif // __ROBOT_BACKUP_H
