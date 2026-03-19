/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-03-19 12:53:56
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-19 20:43:34
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Inc\robot_backup.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: Xiang xin wang wxinxiang8@gmail.com
 * @Date: 2026-03-15 15:20:46
 * @LastEditors: Xiang xin wang wxinxiang8@gmail.com
 * @LastEditTime: 2026-03-15 21:21:36
 * @FilePath: \MDK-ARMd:\robot fighting\robot\Core\Inc\robot_backup.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __ROBOT_BACKUP_H
#define __ROBOT_BACKUP_H

#include "main.h"
#include <stdbool.h>

#define BACKUP_SPIN_TIME_MS      500
#define BACKUP_FORWARD_TIME_MS   800
#define BACKUP_BACK_TIME_MS      500

void Backup_Init(void);
void Backup_Update(void);
bool Backup_IsDone(void);

#endif // __ROBOT_BACKUP_H
