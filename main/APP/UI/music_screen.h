/**
 ******************************************************************************
 * @file        music_screen.h
 * @version     V1.0
 * @brief       闊充箰鎾斁鐣岄潰 - 澶存枃浠? ******************************************************************************
 * @attention   绾?UI 妗嗘灦锛屾殏涓嶅惈鎾斁閫昏緫
 *              - 鏇茬洰鍒楄〃锛堝彲婊氬姩锛? *              - 鎾斁杩涘害鏉★紙闈欐€佸崰浣嶏級
 *              - 涓婁竴棣?/ 鎾斁|鏆傚仠 / 涓嬩竴棣? *              - 闊抽噺 - / +
 *              - 杩斿洖涓婚〉
 ******************************************************************************
 */

#ifndef __MUSIC_SCREEN_H__
#define __MUSIC_SCREEN_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *create_music_screen(void);
void      delete_music_screen(void);
void      music_screen_set_active(bool active);
bool      music_screen_is_playing(void);
void      music_screen_pause_playback(const char *reason);
void      music_screen_interrupt_playback(const char *reason);

#ifdef __cplusplus
}
#endif

#endif /* __MUSIC_SCREEN_H__ */
