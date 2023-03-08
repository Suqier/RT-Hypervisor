/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-28     Suqier       first version
 */

#ifndef __HYP_DEBUG_H__
#define __HYP_DEBUG_H__

#include <rtthread.h>

typedef enum 
{
    HYP_LOG_ERR = 0,    /* Error   */
    HYP_LOG_WARN,       /* Warning */
    HYP_LOG_INFO,       /* Info    */
    HYP_LOG_DEBUG       /* Debug   */
}HYP_LOG_LVL;

#define HYP_LOG_LEVEL   HYP_LOG_DEBUG

#define NONE         "\033[m"
#define LIGHT_RED    "\033[1;31m"       /* error   */
#define LIGHT_GREEN  "\033[1;32m"       /* success */
#define YELLOW       "\033[1;33m"       /* warning */
#define WHITE        "\033[1;37m"       /* info    */
#define LIGHT_BLUE   "\033[1;34m"       /* debug   */

#define hyp_err(fmt, ...)   rt_kprintf(LIGHT_RED "[ERROR] "fmt"\n" NONE, ##__VA_ARGS__)

#define hyp_warn(fmt, ...)   \
do{                          \
    if (HYP_LOG_LEVEL >= HYP_LOG_WARN)  \
        rt_kprintf(YELLOW "[WARN_] "fmt"\n" NONE, ##__VA_ARGS__); \
}while(0)

#define hyp_info(fmt, ...)  \
do{                         \
    if (HYP_LOG_LEVEL >= HYP_LOG_INFO)  \
        rt_kprintf(WHITE "[INFO_] "fmt"\n" NONE, ##__VA_ARGS__); \
}while(0)

#define hyp_debug(fmt, ...)  \
do{                          \
    if (HYP_LOG_LEVEL >= HYP_LOG_DEBUG)  \
        rt_kprintf(LIGHT_BLUE "[DEBUG] "fmt"\n" NONE, ##__VA_ARGS__); \
}while(0)

#endif  /* __HYP_DEBUG_H__ */
