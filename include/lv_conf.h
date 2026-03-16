/**
 * @file lv_conf.h
 * Configuration file for LVGL
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/

#define LV_COLOR_DEPTH 16

/*====================
   MEMORY SETTINGS
 *====================*/

#define LV_MEM_SIZE (32U * 1024U)          /*Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB)*/

/*====================
   HAL SETTINGS
 *====================*/

#define LV_DISP_DEF_REFR_PERIOD 30      /*Display refresh period in milliseconds*/

/*====================
   WIDGET USAGE
 *====================*/

#define LV_USE_ANIMIMG 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1

#endif /*LV_CONF_H*/