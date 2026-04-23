/**
 ******************************************************************************
 * @file        music_screen.c
 * @version     V1.0
 * @brief       闂傚倸锕ら崢鏍不娴兼潙绠绘い鎾跺枑閺夊綊鏌ｉ敐鍛劯婵?- 闁诲骸婀遍崑鐔肩嵁閸ヮ剙妫橀柛銉檮椤愪粙鏌ㄥ☉妯煎ⅱ闁?UI闂佹寧绋戦張顒€鈻撹箛娑樼睄闁绘娅曠亸锟犳煛閳ь剚銈ｉ崘锝呬壕闁哄嫬绻掔敮鍡涙煥? ******************************************************************************
 * @attention   闁汇埄鍨伴崯顐︽儑椤掑嫭鏅?00闁?280 缂備焦姊归悧鏇㈡儓閸℃稒鏅悘鐐垫櫕娣囨椽姊洪鍝勫妞ゆ洟浜堕幊?100px闂佹寧绋戦¨鈧紒? *
 *   闂佸磭鎳撴径鍥绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? *   闂? [闂佹剚鍋呴埀?             闂傚倸锕ら崢鏍不?                闂? 婵＄偑鍊涢崺鏍偉?80px
 *   闂佸疇顫夋竟鏇㈠绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? *   闂?                                       闂? *   闂?        闂佸磭鍎ら弻锟犲疾閺屻儱鐓涢柟鑸妽濞呮粓鏌嶉悜妯哄闁哄懏鐓￠崺锟犲箛閵婏附鐝抽梺宕囧劋閸斿繘寮查弻銉ョ厸闁硅埇鍔嶅▍婊堟煃閻戞ê濮€闁哄懏鐓￠崺锟犲箛閵婏附鐝抽梺宕囧劋閸斿繘寮查弻銉ョ厸闁硅埇鍔嶅▍?          闂? *   闂?        闂?  闂佸摜鍋犳禍顒佹櫠閺嶎偂鐒婂〒姘ｅ亾婵為棿鍗冲畷锟犳偐鏉堚晝孝   闂? 200闁?00  闂? *   闂?        闂佸磭鍎ら崺鍐疾閺屻儱鐓涢柟鑸妽濞呮粓鏌嶉悜妯哄闁哄懏鐓￠崺锟犲箛閵婏附鐝抽梺宕囧劋閸斿繘寮查弻銉ョ厸闁硅埇鍔嶅▍婊堟煃閻戞ê濮€闁哄懏鐓￠崺锟犲箛閵婏附鐝抽梺宕囧劋閸斿繘寮查弻銉ョ厸闁硅埇鍔嶅▓?          闂? *   闂?        闂佸搫娲╃亸娆徝烘导鏉戣Е鐎广儱娉﹂悙鐑樻櫖闁割偅绺挎禍锝夋倵濞戞瑯娈ｇ紒?              闂? *   闂?        濠殿喗绻傞張顒佹櫠?/ 婵炴垶鎸婚幑浣烘椤愶附鏅柛顐ｇ箘濮ｅ牓鎮楀☉娆樻缂?           闂? *   闂?                                       闂? *   闂佸疇顫夋竟鏇㈠绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? *   闂? 闂佸搫娲╃亸娆徝烘导鏉戠婵°倕瀚ㄩ埀顒€鍟撮弫宥夊醇濠靛棜顔夊┑鐘诧攻閼圭偓鎱ㄩ埡鍛闁挎稑瀚。濠氭煥?               闂? *   闂? 闂佸磭鎳撴径鍥绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? 闂? *   闂? 闂?闂? 闂佸搫娲╃亸娆徝洪悧鍫⑩枖闁? 闂佺厧婀遍崕銈咃耿瀹曞洠鍋撶憴鍕瀯缂?             闂? 闂? *   闂? 闂?闂? 闂佸搫娲╃亸娆徝洪悧鍫㈩洸? 闂佺厧婀遍崕銈咃耿瀹曞洠鍋撶憴鍕瀯缂? 闂佸啿鐨烽崑?閻熸粎澧楅幐鍛婃櫠閻樿绠绘い鎾跺枑閺?闂? 闂? *   闂? 闂?闂? 闂佸搫娲╃亸娆徝洪悧鍫⑩枖? 闂佺厧婀遍崕銈咃耿瀹曞洠鍋撶憴鍕瀯缂?             闂? 闂? *   闂? 闂佸疇顫夐弻锟犲绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? 闂? *   闂佸疇顫夋竟鏇㈠绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? *   闂? 闂佹娊鈧稖顔夐柡鈧敐澶婄厴濞达絽寮堕弫姘舵煃闁稖顔夐柡鈧敐澶婄厴濞达絽寮堕弫姘舵煃闁稖顔夋俊顐㈢焸閸╃偞鎷呴崣澶嬫殎闂佹娊鈧稖顔夐柡鈧敐澶婄厴濞达絽寮堕弫姘舵煃闁稖顔夐柡鈧敐澶婄厴濞达絽寮堕弫? 闁哄鏅滅粙鎴犫偓瑙勫▕瀵?        闂? *   闂? 00:00                        00:00    闂? *   闂佸疇顫夋竟鏇㈠绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? *   闂? [闂佸啿鐨烽崑鎾绘煃鐏忔牕浜綸     [  闂?闂佸湱铏庨崢浠嬪棘? ]     [闂佸疇顕ч悘鍫ュ蓟閺?     闂? 闂佺鐭囬崘銊у幀闁? *   闂佸疇顫夋竟鏇㈠绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? *   闂? [闂傚倸锕ユ繛濠囧闯?闂佹剚鍘芥俊?            [闂傚倸锕ユ繛濠囧闯?+]        闂? 闂傚倸锕ユ繛濠囧闯閾忚鍋? *   闂佸疇顫夐弻锟犲绩閵忋倕鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺鐐哄焵椤掑嫬鐓橀柍褜鍓熼崺? *             (闁圭厧鐡ㄥú鐔煎磿鐎电硶鍋撴担鍐棈闁糕晛鎳橀幃?home_screen 缂傚倷鑳堕崰宥囩博鐎靛摜涓嶉柨娑樺閸婄偤鏌ㄥ☉妯绘拱妞ゆ帗绻冮妵鍕垂椤愶綇楠?
 ******************************************************************************
 */

#include "music_screen.h"
#include "ai_assistant_ui.h"
#include "ui_theme.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ff.h"
#include "exfuns.h"
#include "wavplay.h"
#include "myes8311.h"
#include "myi2s.h"
#include "sdmmc.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "music_screen";

#define MUSIC_MAX_TRACKS       64
#define MUSIC_TRACK_NAME_LEN   64
#define MUSIC_TRACK_PATH_LEN   256
#define MUSIC_DEFAULT_ARTIST   "SD音乐"

extern __wavctrl wavctrl;
extern esp_err_t i2s_play_next_prev;

/* ==================== 闁诲繒鍋涚换鎰不妞嬪簶鍋撻悽娈挎敯闁?==================== */
static lv_obj_t *s_music_screen   = NULL;

/* 閻熸粎澧楅幐鍛婃櫠閻樿绠绘い鎾跺枑閺夌懓菐閸ワ絽澧插ù?*/
static lv_obj_t *s_cover_img      = NULL;   /* 闁诲繐绻嬬欢姘焽娴兼潙纭€闁绘浜粔鎾煕?*/
static lv_obj_t *s_track_name_lbl = NULL;   /* 闂佸搫娲╃亸娆徝烘导鏉戣Е鐎广儱娉?*/
static lv_obj_t *s_artist_lbl     = NULL;   /* 濠殿喗绻傞張顒佹櫠?*/
static lv_obj_t *s_progress_bar   = NULL;   /* 闁哄鏅滅粙鎴犫偓瑙勫▕瀵?*/
static lv_obj_t *s_time_cur_lbl   = NULL;   /* 閻熸粎澧楅幐鍛婃櫠閻樿绫嶉柛顐ｆ礃閿?*/
static lv_obj_t *s_time_tot_lbl   = NULL;   /* 闂佽鍓涚划顖氼渻閸岀偞鈷?*/
static lv_obj_t *s_play_btn_lbl   = NULL;   /* 闂佸湱铏庨崢浠嬪棘?闂佸搫妫楅崐鍛婄閻樿绠板鑸靛姈鐏忥箓鏌″鍛窛妞?*/
static lv_obj_t *s_vol_lbl        = NULL;   /* 闂傚倸锕ユ繛濠囧闯濞差亜鍙婇柛鎾椾椒绮?*/
static lv_obj_t *s_list_cont      = NULL;   /* 闂佸憡鐟ラ崢鏍疾閸洖绀嗘俊銈呭閳ь剙鍟伴埀顒€婀遍幊鎾斥枍?*/

/* SD 闂佸搫娲╃亸娆徝烘导鏉戠婵°倕瀚ㄩ埀?*/
typedef struct {
    char name[MUSIC_TRACK_NAME_LEN];
    char artist[24];
    char duration[8];
    char path[MUSIC_TRACK_PATH_LEN];
} track_info_t;

static track_info_t s_tracks[MUSIC_MAX_TRACKS];
static int s_track_count = 0;

static int   s_current_track = 0;
static bool  s_is_playing    = false;
static int   s_volume        = 60;  /* 0-100 */

/* 闂佸憡甯楅〃澶愬Υ閸愵亝鍋橀悘鐐电摂閸ょ娀鎮归悙鈺傤潐缂佽鲸鐟╅幃浠嬪Ω閵堝洩澹樻俊銈呭€圭湁閻庡灚甯″畷姘跺炊閵娿儱绨ラ梺?*/
static lv_obj_t *s_track_rows[MUSIC_MAX_TRACKS];
static TaskHandle_t s_play_task = NULL;
static TaskHandle_t s_backend_task = NULL;
static lv_timer_t  *s_progress_timer = NULL;
static volatile bool s_play_task_exit = false;
static volatile int  s_pending_track = -1;
static volatile bool s_switch_req = false;
static volatile bool s_track_list_dirty = false;
static volatile bool s_backend_init_running = false;
static bool s_backend_inited = false;
static bool s_exfuns_ready = false;
static bool s_audio_ready = false;
static bool s_sd_ready = false;
static bool s_tracks_scanned = false;

#define MUSIC_PROGRESS_PERIOD_ACTIVE_MS 500
#define MUSIC_PROGRESS_PERIOD_IDLE_MS   1500

/* ==================== 闂佸搫妫欓悧鐐寸珶婵犲啰鈻斿┑鐘冲嚬閺?==================== */
#define MUSIC_SCREEN_BG        lv_color_hex(0xFDF6E3)
#define MUSIC_LEFT_PANEL_BG    lv_color_hex(0xF7ECD8)
#define MUSIC_RIGHT_PANEL_BG   lv_color_hex(0xF5E6CF)
#define MUSIC_LIST_BG          lv_color_hex(0xF9EEDC)
#define MUSIC_ROW_BG_ODD       lv_color_hex(0xF7EAD3)
#define MUSIC_ROW_BG_EVEN      lv_color_hex(0xF3E3C9)
#define MUSIC_ROW_ACTIVE_BG    lv_color_hex(0xD3A070)
#define MUSIC_COVER_BG         lv_color_hex(0xC8A173)
#define MUSIC_PROGRESS_BG      lv_color_hex(0xE8D8BF)
#define MUSIC_PROGRESS_FG      lv_color_hex(0xB8875D)
#define MUSIC_BTN_PRIMARY_BG   lv_color_hex(0xB8875D)
#define MUSIC_BTN_SECONDARY_BG lv_color_hex(0xD8B891)
#define MUSIC_TEXT_MAIN        lv_color_hex(0x5C4B43)
#define MUSIC_TEXT_SUB         lv_color_hex(0x7E6A5D)

/* 闂佸憡鎸哥粔鍫曟偪閸℃鐝堕柣妤€鐗婇～?*/
static void create_track_list(lv_obj_t *parent);
static void refresh_now_playing(void);

/* ==================== 闂佸憡鍔曢幊姗€宕曢幘顔藉亹闁煎摜顣介崑鎾存媴鐟欏嫮鍑介梺?==================== */

static void set_text_color_recursive(lv_obj_t *obj, lv_color_t color)
{
    int child_cnt = lv_obj_get_child_cnt(obj);
    for (int i = 0; i < child_cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        lv_obj_set_style_text_color(child, color, LV_PART_MAIN);
        set_text_color_recursive(child, color);
    }
}

static void music_backend_init_once(void)
{
    if (s_backend_inited && s_audio_ready && s_exfuns_ready && s_sd_ready) {
        return;
    }

    esp_err_t ret = ESP_OK;

    /* 婵炴潙鍚嬮敋闁告ɑ绋掑鍕吋閸ャ劍娈?voice_pipeline_init 閻庣懓鎲¤ぐ鍐偩椤掑嫬绠ｉ柟閭﹀枟閻?I2S/ES8311闂佹寧绋戦惌鍌涘閳哄懎绀傜€广儱顦▍銏狀熆鐠鸿櫣孝闁搞劌鐏氶幈銊р偓锝庝簻椤曆囨倵娴ｅ啫顥嶉柛銇卞啰鏆﹂柍鍝勫€婚惃?*/
    if (tx_handle && rx_handle && myes8311_get_codec()) {
        s_audio_ready = true;
    } else {
        if (!tx_handle || !rx_handle) {
            ret = myi2s_init();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "myi2s_init failed: %s", esp_err_to_name(ret));
            }
        }

        if (ret == ESP_OK && !myes8311_get_codec()) {
            ret = myes8311_init();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "myes8311_init failed: %s", esp_err_to_name(ret));
            }
        }

        s_audio_ready = (ret == ESP_OK && tx_handle && rx_handle && myes8311_get_codec());
    }

    if (s_audio_ready) {
        gpio_set_level(PA_CTRL_GPIO_PIN, 1);   /* 闂佺懓鐏氶幐鍝ユ閹达箑绐楅柛銉戝啰鏋?*/
    }

    if (sdmmc_mount_flag != 0x01) {
        ret = sdmmc_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "sdmmc_init failed: %s", esp_err_to_name(ret));
        }
    }

    if (exfuns_init() == 0) {
        s_exfuns_ready = true;
    } else {
        ESP_LOGE(TAG, "exfuns_init failed");
    }

    FF_DIR dir;
    FRESULT fr = f_opendir(&dir, "0:/MUSIC");
    if (fr == FR_OK) {
        s_sd_ready = true;
        f_closedir(&dir);
    } else {
        s_sd_ready = false;
        ESP_LOGW(TAG, "SD/FATFS not ready, open 0:/MUSIC failed: %d", fr);
    }

    s_backend_inited = true;
    ESP_LOGI(TAG, "Music backend init done: audio=%d exfuns=%d sd=%d",
             (int)s_audio_ready, (int)s_exfuns_ready, (int)s_sd_ready);
}

static bool track_is_playable(int idx)
{
    return (idx >= 0 && idx < s_track_count && s_tracks[idx].path[0] != '\0');
}

static char *trim_ascii_space(char *s)
{
    char *start = s;
    while (*start == ' ' || *start == '\t') {
        start++;
    }

    if (start != s) {
        memmove(s, start, strlen(start) + 1U);
    }

    size_t len = strlen(s);
    while (len > 0U && (s[len - 1U] == ' ' || s[len - 1U] == '\t')) {
        s[len - 1U] = '\0';
        len--;
    }
    return s;
}

static void fill_track_name_artist_from_filename(const char *filename, track_info_t *track)
{
    char base[MUSIC_TRACK_NAME_LEN];
    char parsed_base[MUSIC_TRACK_NAME_LEN];
    size_t copy_len = strnlen(filename, sizeof(base) - 1U);
    memcpy(base, filename, copy_len);
    base[copy_len] = '\0';

    char *dot = strrchr(base, '.');
    if (dot) {
        *dot = '\0';
    }

    trim_ascii_space(base);
    size_t parsed_len = strnlen(base, sizeof(parsed_base) - 1U);
    memcpy(parsed_base, base, parsed_len);
    parsed_base[parsed_len] = '\0';

    char *sep = strstr(base, " - ");
    size_t sep_len = 3U;
    const char *sep_type = "none";
    const char *fallback_reason = "separator not found";
    if (sep) {
        sep_type = " - ";
    }
    if (!sep) {
        sep = strchr(base, '-');
        sep_len = 1U;
        if (sep) {
            sep_type = "-";
        }
    }
    if (!sep) {
        sep = strchr(base, '_');
        sep_len = 1U;
        if (sep) {
            sep_type = "_";
        }
    }

    if (sep) {
        *sep = '\0';
        char *artist = trim_ascii_space(base);
        char *name = trim_ascii_space(sep + sep_len);
        if (artist[0] != '\0' && name[0] != '\0') {
            snprintf(track->artist, sizeof(track->artist), "%s", artist);
            snprintf(track->name, sizeof(track->name), "%s", name);
            ESP_LOGI(TAG,
                     "Parse WAV ok: file=%s base=%s sep=%s -> artist=%s, name=%s",
                     filename,
                     parsed_base,
                     sep_type,
                     track->artist,
                     track->name);
            return;
        }
        fallback_reason = "artist or name empty after split";
    }

    snprintf(track->artist, sizeof(track->artist), "%s", MUSIC_DEFAULT_ARTIST);
    snprintf(track->name, sizeof(track->name), "%s", (base[0] != '\0') ? base : "未知歌曲");
    ESP_LOGI(TAG,
             "Parse WAV fallback: file=%s base=%s sep=%s reason=%s -> artist=%s, name=%s",
             filename,
             parsed_base,
             sep_type,
             fallback_reason,
             track->artist,
             track->name);
}

static void build_track_path(const char *filename, char *out, size_t out_size)
{
    static const char prefix[] = "0:/MUSIC/";
    const size_t prefix_len = sizeof(prefix) - 1U;

    if (out_size == 0U) {
        return;
    }

    if (out_size <= prefix_len) {
        out[0] = '\0';
        return;
    }

    memcpy(out, prefix, prefix_len);

    size_t i = 0U;
    const size_t max_copy = out_size - prefix_len - 1U;
    while (i < max_copy && filename[i] != '\0') {
        out[prefix_len + i] = filename[i];
        i++;
    }
    out[prefix_len + i] = '\0';

    if (filename[i] != '\0') {
        /* 文件名过长时截断，避免路径 snprintf 触发 Werror=format-truncation */
        ESP_LOGW(TAG, "Track path truncated: %s", filename);
    }
}

static void sec_to_mmss(uint32_t sec, char out[8])
{
    if (sec > 5999U) {
        sec = 5999U;  /* UI 婵炲濮撮幊蹇撐熸径宀€鐭?MM:SS闂佹寧绋戦惌鍌涘閳哄懎绀傜€广儱娲ㄦ径鍕煕閹邦厼绲婚悘蹇ｅ灦楠炲顢旈崨顔惧姺 */
    }
    uint32_t m = sec / 60;
    uint32_t s = sec % 60;
    out[0] = (char)('0' + (m / 10U));
    out[1] = (char)('0' + (m % 10U));
    out[2] = ':';
    out[3] = (char)('0' + (s / 10U));
    out[4] = (char)('0' + (s % 10U));
    out[5] = '\0';
}

static void ensure_placeholder_tracks(void)
{
    if (s_track_count > 0) {
        return;
    }

    memset(s_tracks, 0, sizeof(s_tracks));
    snprintf(s_tracks[0].name, MUSIC_TRACK_NAME_LEN, "%s", "加载歌曲列表...");
    snprintf(s_tracks[0].artist, sizeof(s_tracks[0].artist), "%s", "请稍候");
    snprintf(s_tracks[0].duration, sizeof(s_tracks[0].duration), "%s", "--:--");
    s_tracks[0].path[0] = '\0';
    s_track_count = 1;
    s_current_track = 0;
}

static void scan_sd_tracks(void)
{
    FF_DIR dir;
    FILINFO fno;
    FRESULT fr;

    s_track_count = 0;
    memset(s_tracks, 0, sizeof(s_tracks));

    fr = f_opendir(&dir, "0:/MUSIC");
    if (fr != FR_OK) {
        s_sd_ready = false;
        ESP_LOGW(TAG, "Open 0:/MUSIC failed: %d", fr);
    } else {
        s_sd_ready = true;
        while (s_track_count < MUSIC_MAX_TRACKS) {
            fr = f_readdir(&dir, &fno);
            if (fr != FR_OK || fno.fname[0] == '\0') {
                break;
            }
            if (fno.fattrib & AM_DIR) {
                continue;
            }
            if (exfuns_file_type(fno.fname) != T_WAV) {
                continue;
            }

            fill_track_name_artist_from_filename(fno.fname, &s_tracks[s_track_count]);
            // /* 临时方案: 先固定显示为“歌曲N”，避免文件名解析问题影响 UI */
            // snprintf(s_tracks[s_track_count].name,
            //          MUSIC_TRACK_NAME_LEN,
            //          "歌曲%d",
            //          s_track_count + 1);
            // snprintf(s_tracks[s_track_count].artist,
            //          sizeof(s_tracks[s_track_count].artist),
            //          "%s",
            //          MUSIC_DEFAULT_ARTIST);
            snprintf(s_tracks[s_track_count].duration, sizeof(s_tracks[s_track_count].duration), "--:--");
            build_track_path(fno.fname,
                             s_tracks[s_track_count].path,
                             sizeof(s_tracks[s_track_count].path));
            ESP_LOGI(TAG, "Track[%d] file=%s -> artist=%s, name=%s",
                     s_track_count,
                     fno.fname,
                     s_tracks[s_track_count].artist,
                     s_tracks[s_track_count].name);
            s_track_count++;
        }
        f_closedir(&dir);
    }

    if (s_track_count == 0) {
        snprintf(s_tracks[0].name, MUSIC_TRACK_NAME_LEN, "%s", "未找到音乐");
        snprintf(s_tracks[0].artist, sizeof(s_tracks[0].artist), "%s", "请放到0:/MUSIC");
        snprintf(s_tracks[0].duration, sizeof(s_tracks[0].duration), "--:--");
        s_tracks[0].path[0] = '\0';
        s_track_count = 1;
    }

    s_current_track = 0;
    s_tracks_scanned = true;
}

static void rebuild_track_list_ui(void)
{
    if (!s_list_cont) {
        return;
    }

    for (int i = 0; i < MUSIC_MAX_TRACKS; i++) {
        s_track_rows[i] = NULL;
    }
    lv_obj_clean(s_list_cont);
    create_track_list(s_list_cont);
    refresh_now_playing();
}

static void music_backend_init_task(void *arg)
{
    (void)arg;
    s_backend_init_running = true;

    music_backend_init_once();

    if (s_audio_ready && s_exfuns_ready && s_sd_ready) {
        scan_sd_tracks();
        s_track_list_dirty = true;
    }

    s_backend_init_running = false;
    s_backend_task = NULL;
    vTaskDelete(NULL);
}

static void ensure_backend_init_task(void)
{
    if (s_backend_inited && s_audio_ready && s_exfuns_ready && s_sd_ready && s_tracks_scanned) {
        return;
    }
    if (s_backend_task != NULL || s_backend_init_running) {
        return;
    }

    BaseType_t ok = xTaskCreate(music_backend_init_task, "music_backend", 6144, NULL, 5, &s_backend_task);
    if (ok != pdPASS) {
        s_backend_task = NULL;
        ESP_LOGE(TAG, "Create music_backend task failed");
    }
}

static void music_play_task(void *arg)
{
    (void)arg;

    while (!s_play_task_exit) {
        if (!s_is_playing) {
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        int idx = (s_pending_track >= 0) ? s_pending_track : s_current_track;
        if (!track_is_playable(idx)) {
            s_is_playing = false;
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        s_current_track = idx;
        s_pending_track = -1;
        s_switch_req = false;
        ESP_LOGI(TAG, "Play: %s", s_tracks[idx].path);

        (void)wav_play_song((uint8_t *)s_tracks[idx].path);

        if (s_play_task_exit) {
            break;
        }

        bool switched = s_switch_req || (s_pending_track >= 0) || (i2s_play_next_prev == ESP_OK);
        i2s_play_next_prev = ESP_FAIL;

        if (!s_is_playing) {
            continue;
        }

        if (!switched && s_track_count > 0) {
            s_current_track = (s_current_track + 1) % s_track_count;
            s_pending_track = s_current_track;
        }
    }

    audio_stop();
    s_play_task = NULL;
    vTaskDelete(NULL);
}

static void ensure_play_task(void)
{
    if (s_play_task != NULL) {
        return;
    }
    BaseType_t ok = xTaskCreate(music_play_task, "music_play", 6144, NULL, 5, &s_play_task);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Create music_play task failed");
        s_play_task = NULL;
        s_is_playing = false;
    }
}

static void sync_progress_to_ui(void)
{
    char cur[8];
    char tot[8];
    uint32_t cur_sec = wavctrl.cursec;
    uint32_t tot_sec = wavctrl.totsec;

    if (tot_sec > 0) {
        sec_to_mmss(cur_sec, cur);
        sec_to_mmss(tot_sec, tot);
        if (s_time_cur_lbl) {
            lv_label_set_text(s_time_cur_lbl, cur);
        }
        if (s_time_tot_lbl) {
            lv_label_set_text(s_time_tot_lbl, tot);
        }
        if (s_progress_bar) {
            int pct = (int)((cur_sec * 100U) / tot_sec);
            lv_slider_set_value(s_progress_bar, pct, LV_ANIM_OFF);
        }
        if (s_current_track >= 0 && s_current_track < s_track_count) {
            snprintf(s_tracks[s_current_track].duration, sizeof(s_tracks[s_current_track].duration), "%s", tot);
        }
    }
    if (s_play_btn_lbl) {
        lv_label_set_text(s_play_btn_lbl, s_is_playing ? "暂停" : "播放");
    }
}

static void progress_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_music_screen) {
        return;
    }

    if (s_track_list_dirty) {
        s_track_list_dirty = false;
        rebuild_track_list_ui();
    }

    sync_progress_to_ui();
}

static void refresh_track_list_highlight(void)
{
    for (int i = 0; i < s_track_count; i++) {
        if (!s_track_rows[i]) continue;
        if (i == s_current_track) {
            lv_obj_set_style_bg_color(s_track_rows[i], MUSIC_ROW_ACTIVE_BG, LV_PART_MAIN);
            set_text_color_recursive(s_track_rows[i], THEME_TEXT_ON_DARK);
        } else {
            lv_color_t bg = (i % 2 == 0) ? MUSIC_ROW_BG_ODD : MUSIC_ROW_BG_EVEN;
            lv_obj_set_style_bg_color(s_track_rows[i], bg, LV_PART_MAIN);
            set_text_color_recursive(s_track_rows[i], MUSIC_TEXT_MAIN);
        }
    }
}

static void refresh_now_playing(void)
{
    if (s_track_count <= 0) {
        return;
    }
    if (s_current_track < 0 || s_current_track >= s_track_count) {
        s_current_track = 0;
    }

    if (s_track_name_lbl)
        lv_label_set_text(s_track_name_lbl, s_tracks[s_current_track].name);
    if (s_artist_lbl)
        lv_label_set_text(s_artist_lbl, s_tracks[s_current_track].artist);
    if (s_time_tot_lbl)
        lv_label_set_text(s_time_tot_lbl, s_tracks[s_current_track].duration);
    if (s_time_cur_lbl)
        lv_label_set_text(s_time_cur_lbl, "00:00");
    if (s_progress_bar)
        lv_slider_set_value(s_progress_bar, 0, LV_ANIM_OFF);
    if (s_play_btn_lbl)
        lv_label_set_text(s_play_btn_lbl, s_is_playing ? "暂停" : "播放");
}

/* ==================== 婵炲瓨绮岄鍕枎閵忋倕鐐婇柣鎰濞?==================== */

static void back_btn_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Back to home");
    ui_switch_to_home();
}

static void track_row_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= s_track_count) {
        return;
    }
    ESP_LOGI(TAG, "Track selected: %d (%s)", idx, s_tracks[idx].name);
    s_current_track = idx;
    s_pending_track = idx;

    if (s_is_playing) {
        s_switch_req = true;
        i2s_play_next_prev = ESP_OK;
    } else {
        s_switch_req = (s_play_task != NULL);
    }

    refresh_track_list_highlight();
    refresh_now_playing();
}

static void play_pause_cb(lv_event_t *e)
{
    if (!s_audio_ready || !s_exfuns_ready || !s_sd_ready) {
        ensure_backend_init_task();
        ESP_LOGW(TAG, "Music backend not ready: audio=%d exfuns=%d sd=%d",
                 (int)s_audio_ready, (int)s_exfuns_ready, (int)s_sd_ready);
        return;
    }

    if (!s_is_playing) {
        if (!track_is_playable(s_current_track)) {
            ESP_LOGW(TAG, "No playable WAV in 0:/MUSIC");
            return;
        }

        if (s_pending_track < 0) {
            s_pending_track = s_current_track;
        }

        ensure_play_task();
        if (s_play_task) {
            s_is_playing = true;
            audio_start();
            if (s_switch_req) {
                i2s_play_next_prev = ESP_OK;
            }
        }
    } else {
        s_is_playing = false;
        audio_stop();
    }

    ESP_LOGI(TAG, "Play/Pause -> %s", s_is_playing ? "playing" : "paused");
    if (s_play_btn_lbl)
        lv_label_set_text(s_play_btn_lbl, s_is_playing ? "暂停" : "播放");
}

static void prev_track_cb(lv_event_t *e)
{
    if (s_track_count <= 0) {
        return;
    }

    if (s_current_track > 0) s_current_track--;
    else s_current_track = s_track_count - 1;

    s_pending_track = s_current_track;
    if (s_is_playing) {
        s_switch_req = true;
        i2s_play_next_prev = ESP_OK;
    } else {
        s_switch_req = (s_play_task != NULL);
    }

    refresh_track_list_highlight();
    refresh_now_playing();
    ESP_LOGI(TAG, "Prev track: %d", s_current_track);
}

static void next_track_cb(lv_event_t *e)
{
    if (s_track_count <= 0) {
        return;
    }

    s_current_track = (s_current_track + 1) % s_track_count;
    s_pending_track = s_current_track;
    if (s_is_playing) {
        s_switch_req = true;
        i2s_play_next_prev = ESP_OK;
    } else {
        s_switch_req = (s_play_task != NULL);
    }

    refresh_track_list_highlight();
    refresh_now_playing();
    ESP_LOGI(TAG, "Next track: %d", s_current_track);
}

static void vol_down_cb(lv_event_t *e)
{
    if (s_volume >= 10) s_volume -= 10;
    ESP_LOGI(TAG, "Volume: %d", s_volume);
    if (s_vol_lbl) lv_label_set_text_fmt(s_vol_lbl, "音量 %d%%", s_volume);
    esp_err_t ret = myes8311_set_volume(s_volume);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Set volume failed: %s", esp_err_to_name(ret));
    }
}

static void vol_up_cb(lv_event_t *e)
{
    if (s_volume <= 90) s_volume += 10;
    ESP_LOGI(TAG, "Volume: %d", s_volume);
    if (s_vol_lbl) lv_label_set_text_fmt(s_vol_lbl, "音量 %d%%", s_volume);
    esp_err_t ret = myes8311_set_volume(s_volume);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Set volume failed: %s", esp_err_to_name(ret));
    }
}

static void square_obj_size_sync_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_coord_t w = lv_obj_get_width(obj);
    if (w > 0 && lv_obj_get_height(obj) != w) {
        lv_obj_set_height(obj, w);
    }
}

static void apply_music_button_style(lv_obj_t *btn, bool primary)
{
    lv_obj_set_style_radius(btn, 24, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, primary ? MUSIC_BTN_PRIMARY_BG : MUSIC_BTN_SECONDARY_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 14, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0xBBA78F), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn, 4, LV_PART_MAIN);
}

/* ==================== 闂佸搫娲╃亸娆徝烘导鏉戠婵°倕瀚ㄩ埀?==================== */
static void create_track_list(lv_obj_t *parent)
{
    bool compact = (LV_VER_RES <= 620);

    for (int i = 0; i < s_track_count; i++) {
        lv_obj_t *row = lv_obj_create(parent);
        s_track_rows[i] = row;
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, compact ? 74 : 92);
        lv_obj_set_style_radius(row, compact ? 16 : 20, LV_PART_MAIN);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_left(row, compact ? 12 : 16, LV_PART_MAIN);
        lv_obj_set_style_pad_right(row, compact ? 12 : 16, LV_PART_MAIN);
        lv_obj_set_style_pad_top(row, compact ? 8 : 12, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(row, compact ? 8 : 12, LV_PART_MAIN);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *icon_lbl = lv_label_create(row);
        lv_label_set_text(icon_lbl, "听");
        lv_obj_set_style_text_font(icon_lbl, THEME_FONT_CN, LV_PART_MAIN);

        lv_obj_t *text_col = lv_obj_create(row);
        lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(text_col, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(text_col, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_row(text_col, compact ? 1 : 2, LV_PART_MAIN);
        lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(text_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_flex_grow(text_col, 1);
        lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *name_lbl = lv_label_create(text_col);
        lv_label_set_text(name_lbl, s_tracks[i].name);
        lv_obj_set_style_text_font(name_lbl, THEME_FONT_CN, LV_PART_MAIN);
        lv_obj_set_width(name_lbl, lv_pct(100));
        lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);

        lv_obj_t *art_lbl = lv_label_create(text_col);
        lv_label_set_text(art_lbl, s_tracks[i].artist);
        lv_obj_set_style_text_font(art_lbl, THEME_FONT_CN, LV_PART_MAIN);
        lv_obj_set_style_text_color(art_lbl, MUSIC_TEXT_SUB, LV_PART_MAIN);
        lv_obj_set_width(art_lbl, lv_pct(100));
        lv_label_set_long_mode(art_lbl, LV_LABEL_LONG_DOT);

        lv_obj_t *dur_lbl = lv_label_create(row);
        lv_label_set_text(dur_lbl, s_tracks[i].duration);
        lv_obj_set_style_text_font(dur_lbl, THEME_FONT_CN, LV_PART_MAIN);
        lv_obj_set_style_text_color(dur_lbl, MUSIC_TEXT_SUB, LV_PART_MAIN);
        lv_obj_set_style_translate_y(dur_lbl, compact ? 1 : 2, LV_PART_MAIN);

        lv_obj_add_event_cb(row, track_row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    refresh_track_list_highlight();
}

static void create_left_panel(lv_obj_t *parent)
{
    bool compact = (LV_VER_RES <= 620);

    lv_obj_t *left = lv_obj_create(parent);
    lv_obj_set_width(left, lv_pct(45));
    lv_obj_set_height(left, lv_pct(100));
    lv_obj_set_style_bg_color(left, MUSIC_LEFT_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(left, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(left, 28, LV_PART_MAIN);
    lv_obj_set_style_border_width(left, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(left, 18, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(left, lv_color_hex(0xCCB8A0), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(left, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_pad_all(left, compact ? 12 : 18, LV_PART_MAIN);
    lv_obj_set_style_pad_row(left, compact ? 8 : 12, LV_PART_MAIN);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *header_row = lv_obj_create(left);
    lv_obj_set_width(header_row, lv_pct(100));
    lv_obj_set_height(header_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back_btn = lv_btn_create(header_row);
    lv_obj_set_width(back_btn, lv_pct(30));
    lv_obj_set_height(back_btn, compact ? 46 : 52);
    apply_music_button_style(back_btn, false);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "返回");
    lv_obj_set_style_text_font(back_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_lbl, MUSIC_TEXT_MAIN, LV_PART_MAIN);
    lv_obj_center(back_lbl);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *title_lbl = lv_label_create(header_row);
    lv_label_set_text(title_lbl, "音乐播放");
    lv_obj_set_style_text_font(title_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_lbl, MUSIC_TEXT_MAIN, LV_PART_MAIN);

    s_vol_lbl = lv_label_create(header_row);
    lv_label_set_text_fmt(s_vol_lbl, "音量 %d%%", s_volume);
    lv_obj_set_style_text_font(s_vol_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_vol_lbl, MUSIC_TEXT_SUB, LV_PART_MAIN);

    s_cover_img = lv_obj_create(left);
    lv_obj_set_width(s_cover_img, compact ? lv_pct(50) : lv_pct(62));
    lv_obj_set_style_bg_color(s_cover_img, MUSIC_COVER_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_cover_img, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s_cover_img, 22, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_cover_img, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_cover_img, 24, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(s_cover_img, lv_color_hex(0xC9AE8A), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(s_cover_img, LV_OPA_30, LV_PART_MAIN);
    lv_obj_clear_flag(s_cover_img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_cover_img, square_obj_size_sync_cb, LV_EVENT_SIZE_CHANGED, NULL);

    lv_obj_t *cover_note = lv_label_create(s_cover_img);
    lv_label_set_text(cover_note, "音乐");
    lv_obj_set_style_text_font(cover_note, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(cover_note, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_center(cover_note);

    s_track_name_lbl = lv_label_create(left);
    lv_obj_set_width(s_track_name_lbl, lv_pct(92));
    lv_label_set_long_mode(s_track_name_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(s_track_name_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_track_name_lbl, MUSIC_TEXT_MAIN, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_track_name_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    s_artist_lbl = lv_label_create(left);
    lv_obj_set_width(s_artist_lbl, lv_pct(86));
    lv_label_set_long_mode(s_artist_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(s_artist_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_artist_lbl, MUSIC_TEXT_SUB, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_artist_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_t *progress_wrap = lv_obj_create(left);
    lv_obj_set_width(progress_wrap, lv_pct(92));
    lv_obj_set_height(progress_wrap, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(progress_wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(progress_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(progress_wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(progress_wrap, compact ? 6 : 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(progress_wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(progress_wrap, LV_OBJ_FLAG_SCROLLABLE);

    s_progress_bar = lv_slider_create(progress_wrap);
    lv_obj_set_width(s_progress_bar, lv_pct(100));
    lv_obj_set_height(s_progress_bar, compact ? 14 : 18);
    lv_slider_set_range(s_progress_bar, 0, 100);
    lv_slider_set_value(s_progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_progress_bar, MUSIC_PROGRESS_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_progress_bar, MUSIC_PROGRESS_FG, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_progress_bar, lv_color_hex(0xB07C53), LV_PART_KNOB);
    lv_obj_set_style_radius(s_progress_bar, compact ? 14 : 18, LV_PART_MAIN);
    lv_obj_set_style_radius(s_progress_bar, compact ? 14 : 18, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_progress_bar, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_progress_bar, 2, LV_PART_KNOB);

    lv_obj_t *time_row = lv_obj_create(progress_wrap);
    lv_obj_set_width(time_row, lv_pct(100));
    lv_obj_set_height(time_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(time_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(time_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(time_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(time_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(time_row, LV_OBJ_FLAG_SCROLLABLE);

    s_time_cur_lbl = lv_label_create(time_row);
    lv_label_set_text(s_time_cur_lbl, "00:00");
    lv_obj_set_style_text_font(s_time_cur_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_time_cur_lbl, MUSIC_TEXT_SUB, LV_PART_MAIN);

    s_time_tot_lbl = lv_label_create(time_row);
    lv_obj_set_style_text_font(s_time_tot_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_time_tot_lbl, MUSIC_TEXT_SUB, LV_PART_MAIN);

    lv_obj_t *ctrl_row = lv_obj_create(left);
    lv_obj_set_width(ctrl_row, lv_pct(100));
    lv_obj_set_height(ctrl_row, compact ? 72 : 88);
    lv_obj_set_style_bg_opa(ctrl_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ctrl_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ctrl_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(ctrl_row, compact ? 8 : 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(ctrl_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ctrl_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *prev_btn = lv_btn_create(ctrl_row);
    lv_obj_set_height(prev_btn, lv_pct(100));
    lv_obj_set_flex_grow(prev_btn, 1);
    apply_music_button_style(prev_btn, false);
    lv_obj_t *prev_lbl = lv_label_create(prev_btn);
    lv_label_set_text(prev_lbl, "上一首");
    lv_obj_set_style_text_font(prev_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(prev_lbl, MUSIC_TEXT_MAIN, LV_PART_MAIN);
    lv_obj_center(prev_lbl);
    lv_obj_add_event_cb(prev_btn, prev_track_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *play_btn = lv_btn_create(ctrl_row);
    lv_obj_set_height(play_btn, lv_pct(100));
    lv_obj_set_flex_grow(play_btn, 2);
    apply_music_button_style(play_btn, true);
    s_play_btn_lbl = lv_label_create(play_btn);
    lv_label_set_text(s_play_btn_lbl, "播放");
    lv_obj_set_style_text_font(s_play_btn_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_play_btn_lbl, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_center(s_play_btn_lbl);
    lv_obj_add_event_cb(play_btn, play_pause_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next_btn = lv_btn_create(ctrl_row);
    lv_obj_set_height(next_btn, lv_pct(100));
    lv_obj_set_flex_grow(next_btn, 1);
    apply_music_button_style(next_btn, false);
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, "下一首");
    lv_obj_set_style_text_font(next_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(next_lbl, MUSIC_TEXT_MAIN, LV_PART_MAIN);
    lv_obj_center(next_lbl);
    lv_obj_add_event_cb(next_btn, next_track_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *vol_row = lv_obj_create(left);
    lv_obj_set_width(vol_row, lv_pct(100));
    lv_obj_set_height(vol_row, compact ? 52 : 64);
    lv_obj_set_style_bg_opa(vol_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(vol_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(vol_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(vol_row, compact ? 8 : 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(vol_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(vol_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *vd_btn = lv_btn_create(vol_row);
    lv_obj_set_height(vd_btn, lv_pct(100));
    lv_obj_set_flex_grow(vd_btn, 1);
    apply_music_button_style(vd_btn, false);
    lv_obj_t *vd_lbl = lv_label_create(vd_btn);
    lv_label_set_text(vd_lbl, "音量-");
    lv_obj_set_style_text_font(vd_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(vd_lbl, MUSIC_TEXT_MAIN, LV_PART_MAIN);
    lv_obj_center(vd_lbl);
    lv_obj_add_event_cb(vd_btn, vol_down_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *vu_btn = lv_btn_create(vol_row);
    lv_obj_set_height(vu_btn, lv_pct(100));
    lv_obj_set_flex_grow(vu_btn, 1);
    apply_music_button_style(vu_btn, false);
    lv_obj_t *vu_lbl = lv_label_create(vu_btn);
    lv_label_set_text(vu_lbl, "音量+");
    lv_obj_set_style_text_font(vu_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(vu_lbl, MUSIC_TEXT_MAIN, LV_PART_MAIN);
    lv_obj_center(vu_lbl);
    lv_obj_add_event_cb(vu_btn, vol_up_cb, LV_EVENT_CLICKED, NULL);
}

static void create_right_panel(lv_obj_t *parent)
{
    bool compact = (LV_VER_RES <= 620);

    lv_obj_t *right = lv_obj_create(parent);
    lv_obj_set_width(right, lv_pct(55));
    lv_obj_set_height(right, lv_pct(100));
    lv_obj_set_style_bg_color(right, MUSIC_RIGHT_PANEL_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(right, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(right, 28, LV_PART_MAIN);
    lv_obj_set_style_border_width(right, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(right, 18, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(right, lv_color_hex(0xCCB8A0), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(right, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_pad_all(right, compact ? 10 : 16, LV_PART_MAIN);
    lv_obj_set_style_pad_row(right, compact ? 6 : 10, LV_PART_MAIN);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *title_lbl = lv_label_create(right);
    lv_label_set_text(title_lbl, "歌曲列表");
    lv_obj_set_style_text_font(title_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_lbl, MUSIC_TEXT_MAIN, LV_PART_MAIN);

    lv_obj_t *list_cont = lv_obj_create(right);
    s_list_cont = list_cont;
    lv_obj_set_width(list_cont, lv_pct(100));
    lv_obj_set_flex_grow(list_cont, 1);
    lv_obj_set_style_bg_color(list_cont, MUSIC_LIST_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(list_cont, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(list_cont, 24, LV_PART_MAIN);
    lv_obj_set_style_border_width(list_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(list_cont, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(list_cont, lv_color_hex(0xD6C3AB), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(list_cont, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list_cont, compact ? 8 : 12, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list_cont, compact ? 6 : 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(list_cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_AUTO);

    create_track_list(list_cont);
}

/* ==================== 闂佸憡甯楃粙鎴犵磽閹剧粯顥婇柛蹇撴噽椤斿洭鏌ｉ敐鍛劯婵?==================== */
lv_obj_t *create_music_screen(void)
{
    ESP_LOGI(TAG, "Creating music screen...");

    /* 闂備焦褰冪粔鍫曟偪閸℃稒鍋愰柤鍝ヮ暯閸?*/
    for (int i = 0; i < MUSIC_MAX_TRACKS; i++) s_track_rows[i] = NULL;
    s_is_playing = false;
    s_pending_track = -1;
    s_switch_req = false;
    s_play_task_exit = false;
    ensure_placeholder_tracks();

    s_music_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_music_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(s_music_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_music_screen, MUSIC_SCREEN_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_music_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_music_screen, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_column(s_music_screen, 12, LV_PART_MAIN);
    lv_obj_set_flex_flow(s_music_screen, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_music_screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    create_left_panel(s_music_screen);
    create_right_panel(s_music_screen);

    /* 闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宥冨妽閳绘梻绱掗埀?*/
    refresh_now_playing();

    if (s_progress_timer) {
        lv_timer_del(s_progress_timer);
        s_progress_timer = NULL;
    }
    s_progress_timer = lv_timer_create(progress_timer_cb, MUSIC_PROGRESS_PERIOD_ACTIVE_MS, NULL);

    ensure_backend_init_task();
    sync_progress_to_ui();

    esp_err_t vol_ret = myes8311_set_volume(s_volume);
    if (vol_ret != ESP_OK) {
        ESP_LOGW(TAG, "Init volume failed: %s", esp_err_to_name(vol_ret));
    }

    ESP_LOGI(TAG, "Music screen created OK");
    return s_music_screen;
}

/* ==================== 闂佸憡甯炴繛鈧繛鍛叄濡懘宕楅崨顖ｅ敹闂佷紮绲介惌鍌氼焽?==================== */
void delete_music_screen(void)
{
    s_is_playing = false;
    s_play_task_exit = true;
    s_switch_req = true;
    i2s_play_next_prev = ESP_OK;
    audio_stop();
    s_track_list_dirty = false;
    s_list_cont = NULL;

    if (s_progress_timer) {
        lv_timer_del(s_progress_timer);
        s_progress_timer = NULL;
    }

    if (s_music_screen) {
        lv_obj_del(s_music_screen);
        s_music_screen   = NULL;
        s_cover_img      = NULL;
        s_track_name_lbl = NULL;
        s_artist_lbl     = NULL;
        s_progress_bar   = NULL;
        s_time_cur_lbl   = NULL;
        s_time_tot_lbl   = NULL;
        s_play_btn_lbl   = NULL;
        s_vol_lbl        = NULL;
        for (int i = 0; i < MUSIC_MAX_TRACKS; i++) s_track_rows[i] = NULL;
    }
}

void music_screen_set_active(bool active)
{
    if (!s_progress_timer) {
        return;
    }

    lv_timer_set_period(s_progress_timer,
                        active ? MUSIC_PROGRESS_PERIOD_ACTIVE_MS : MUSIC_PROGRESS_PERIOD_IDLE_MS);
    if (active) {
        lv_timer_ready(s_progress_timer);
    }
}

bool music_screen_is_playing(void)
{
    return s_is_playing;
}

void music_screen_pause_playback(const char *reason)
{
    if (!s_is_playing) {
        return;
    }

    ESP_LOGI(TAG, "Pause music playback: %s", reason ? reason : "gesture");
    s_is_playing = false;
    audio_stop();

    if (lvgl_port_lock(200)) {
        refresh_now_playing();
        lvgl_port_unlock();
    }
}

void music_screen_interrupt_playback(const char *reason)
{
    if (!s_is_playing && s_play_task == NULL) {
        return;
    }

    ESP_LOGI(TAG, "Interrupt music playback: %s", reason ? reason : "gesture");
    s_is_playing = false;
    s_switch_req = true;
    s_pending_track = -1;
    i2s_play_next_prev = ESP_OK;
    wavctrl.cursec = 0;
    audio_stop();

    /* refresh_now_playing 操作 LVGL 对象，必须持锁 */
    if (lvgl_port_lock(200)) {
        refresh_now_playing();
        lvgl_port_unlock();
    }
}
