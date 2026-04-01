
#ifndef __KEY_SCAN_H__
#define __KEY_SCAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

//扫描配置
#define KEY_SCAN_TRIG_EN                    0           //是否使能扫描触发（适用于按需扫描：按键触发后启动扫描，所有按键释放后停止扫描）

//消息配置
#define MSG_PRESS_EN                        1           //按下消息使能
#define MSG_RELEASE_EN                      1           //释放消息使能
#define MSG_SHORT_UP_EN                     1           //短按抬起消息使能
#define MSG_LONG_EN                         1           //长按消息使能
#define MSG_LONG_LONG_EN                    1           //超长按消息使能
#define MSG_LONG_HOLD_EN                    1           //长按保持消息使能
#define MSG_LONG_UP_EN                      1           //长按抬起消息使能
#define MSG_MULT_HIT_EN                     1           //多击消息使能（启用时会降低 MSG_SHORT_UP_EN 的响应速度）

//时间配置
#define KEY_SCAN_PERIOD_MS				    10          //按键扫描周期
#define KEY_TRIG_FILTER_MS			        10          //按下消抖时间（释放消抖时间为：KEY_SCAN_PERIOD_MS）
#define KEY_LONG_CHECK_TIME_MS			    1000        //长按触发时间
#define KEY_LONG_LONG_CHECK_TIME_MS			3000        //超长按触发时间
#define KEY_LONG_HOLD_CHECK_PER_MS		    500         //长按保持触发周期时间
#define KEY_MULT_HIT_CHECK_TIME_MS		    200         //多击检测时间

//按键时间转换为扫描tick数
#define KEY_TIME_TO_TICK(MS)                ((MS) / (KEY_SCAN_PERIOD_MS))

//通过按键消息获取多击次数
#define KEY_GET_MULT_HIT_NUM(MSG)           ((MSG) - KEY_MULT_HIT_2 + 2)

//最大计数值
#define KEY_SCAN_MAX_CNT					(0xFFFFFFFF)

//检查配置
#if KEY_TIME_TO_TICK(KEY_LONG_CHECK_TIME_MS)>KEY_TIME_TO_TICK(KEY_LONG_LONG_CHECK_TIME_MS)
#error "LONG_LONG must be greater than LONG"
#endif

#if (KEY_TIME_TO_TICK(KEY_LONG_LONG_CHECK_TIME_MS)+KEY_TIME_TO_TICK(KEY_LONG_HOLD_CHECK_PER_MS)) >= (KEY_SCAN_MAX_CNT)
#error "KEY_LONG_LONG_CHECK_TIME_MS + KEY_LONG_HOLD_CHECK_PER_MS overflow"
#endif


//返回
typedef enum {
    KEY_SCAN_RET_OK = 0,
    KEY_SCAN_RET_ERR_PARAM,
    KEY_SCAN_RET_ERR_INIT,
    KEY_SCAN_RET_ERR_NO_PORT,
    KEY_SCAN_RET_ERR_STA, 
}enum_KeyScanRet_t;

//按键状态信息
typedef enum {
    KEY_NULL = 0,           //

    KEY_PRESS,              //按下
    KEY_RELEASE,            //抬起

    KEY_SHORT_UP,           //短按抬起
    KEY_LONG,               //长按
    KEY_LONG_HOLD,          //长按保持
    KEY_LONG_LONG,          //长按长按
    KEY_LONG_UP,            //长按抬起

    KEY_MULT_HIT_2,         //双击
    /* 多击 */

    KEY_ERR = 0xFF          //异常消息（异常多击）
}enum_KeyTrigMsg_t;

//按键检测状态
typedef enum {
    KEY_STA_RELEASE = 0,    //释放
    KEY_STA_PRESS,          //按下
    KEY_STA_RELEASE_CHECK,  //释放检测
    KEY_STA_PRESS_CHECK,    //按下检测

    KEY_STA_ERR,            //异常状态
}enum_KeySta_t;

//按键触发状态
typedef enum {
    KEY_TRIG_STA_RELEASE = 0,       //释放
    KEY_TRIG_STA_PRESS,             //按下
}enum_KeyTrigSta_t;

//按键扫描状态
typedef enum {
    KEY_SCAN_DISABLE = 0, 
    KEY_SCAN_ENABLE,
}enum_key_scan_type_t;

//按键扫描回调函数
typedef struct {
    enum_KeyTrigSta_t(*pfn_get_key_sta)(uint8_t port );     //获取按键状态函数
    #if KEY_SCAN_TRIG_EN
    void(*pfn_key_scan_start)( uint32_t per_ms );           //扫描定时器控制启动（按需扫描时使用）
    void(*pfn_key_scan_stop)( void );                       //扫描定时器控制停止（按需扫描时使用）
    #endif
    void(*pfn_key_trig_cb)( uint8_t port, uint8_t msg );    //按键触发回调
}key_scan_cb_t;

//按键定义
typedef struct KeyScanKey
{
    struct KeyScanKey *p_next;      //链表next指针
    uint32_t tick_cnt;              //按键时间
    uint8_t port;                   //按键端口      0~255
    uint8_t scan_flg;               //扫描标志      enum_key_scan_type_t
    uint8_t sta;                    //按键状态      enum_KeySta_t
    uint8_t msg;                    //按键消息      enum_KeyTrigMsg_t
    #if MSG_MULT_HIT_EN
    uint8_t max_hit;                //最大点击数    0~(KEY_TIME_TO_TICK(KEY_ERR-1))
    #endif
}key_scan_key_t;


enum_KeyScanRet_t 
key_scan_init( key_scan_cb_t *pfn_Cb );
void 
key_scan_scan_proc( void );

//（按需扫描时使用）
#if KEY_SCAN_TRIG_EN
void 
key_scan_trig_scan( void );
#else
#define key_scan_trig_scan()
#endif


enum_KeyScanRet_t 
key_scan_key_add( key_scan_key_t *p_key, uint8_t port, uint8_t max_hit );
enum_KeyScanRet_t 
key_scan_key_del( key_scan_key_t *p_key );


#ifdef __cplusplus
}
#endif

#endif

