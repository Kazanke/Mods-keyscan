
#include "key_scan.h"


#define KEY_SCAN_DEBUG_EN                   0               //按键扫描调试

#if KEY_SCAN_DEBUG_EN
    #define KEY_SCAN_DEBUG(...)             printf(__VA_ARGS__)
#else
    #define KEY_SCAN_DEBUG(...)
#endif


//按键扫描状态标志
enum {
    KEY_SCAN_NULL = 0,                                  //按键扫描初始状态  
    KEY_SCAN_BUSY,                                      //按键扫描忙（执行添加/删除/初始化按键）

    KEY_SCAN_READY,                                     //按键扫描就绪
};


// 按键扫描结构体
typedef struct {
    volatile uint8_t sta_flg;                           //按键扫描初状态标志
    uint8_t trig_num;                                   //触发按键数
    key_scan_key_t *key_list;                           //按键链表
    key_scan_cb_t callback;                             //按键扫描回调函数
}key_scan_t;


// 按键扫描结构体初始化值
static key_scan_t g_key_scan = {
    .sta_flg = KEY_SCAN_NULL, 
    .trig_num = 0,
    .key_list = NULL,
    .callback = {
		.pfn_get_key_sta = NULL,
        #if KEY_SCAN_TRIG_EN
        .pfn_key_scan_start = NULL,
        .pfn_key_scan_stop = NULL,
        #endif
        .pfn_key_trig_cb = NULL,
    },
};


/**
 * @brief 释放按键处理（多击处理、扫描标志清除）
 * @param p_key: 按键结构体指针
 * @return void
 */
static inline void 
__key_scan_release_hold( key_scan_key_t *p_key )
{
    // 短按类消息才执行多击判断
    #if MSG_MULT_HIT_EN
    if( ( p_key->max_hit > 1 ) 
     && ( KEY_PRESS == p_key->msg || p_key->msg >= KEY_MULT_HIT_2 ) ) {
        // 多击按键处理
        if( p_key->tick_cnt < KEY_TIME_TO_TICK( KEY_MULT_HIT_CHECK_TIME_MS ) ) {
            // 多击判定时间未到
            return;
        } else {
            if( p_key->msg == KEY_PRESS ) {
                // 触发短按抬起消息
                #if MSG_SHORT_UP_EN
                g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_SHORT_UP );
                #endif
            } else if( KEY_ERR == p_key->msg ) {
                // 异常多击按键抬起

            } else if( p_key->msg >= KEY_MULT_HIT_2 ) {
                // 触发多击消息
                #if MSG_MULT_HIT_EN
                g_key_scan.callback.pfn_key_trig_cb( p_key->port, p_key->msg );
                #endif
            }
        }
    }
    #endif

    //清除扫描标志
    p_key->scan_flg = KEY_SCAN_DISABLE;
    p_key->tick_cnt = 0;
    p_key->msg = KEY_NULL;

    if( g_key_scan.trig_num ) {
        g_key_scan.trig_num--;
    }
}

/**
 * @brief 按键释放处理
 * @param p_key: 按键结构体指针
 * @return void
 */
static inline void 
__key_scan_release_proc( key_scan_key_t *p_key )
{
    // 触发释放消息
    #if MSG_RELEASE_EN
    g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_RELEASE );
    #endif

    if( KEY_PRESS == p_key->msg || p_key->msg >= KEY_MULT_HIT_2 ) {
        // 短按消息处理
        #if MSG_MULT_HIT_EN
        if ( p_key->max_hit == 1 ) {
            // 触发短按释放消息
            #if MSG_SHORT_UP_EN
            g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_SHORT_UP );
            #endif
        } else {
            // 多击按键释放（多击按键释放消息在 __key_scan_release_hold 中处理）
        }
        #else
            #if MSG_SHORT_UP_EN
            g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_SHORT_UP );
            #endif
        #endif
    } else if( KEY_LONG_LONG == p_key->msg || KEY_LONG == p_key->msg ) {
        // 触发长按释放消息
        #if MSG_LONG_UP_EN
        g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_LONG_UP );
        #endif
    }

    //扫描标志统一在 __key_scan_release_hold 中清除
}

/**
 * @brief 按键按下保持处理
 * @param p_key: 按键结构体指针
 * @return void
 */
static inline void 
__key_scan_press_hold( key_scan_key_t *p_key )
{
    if( p_key->tick_cnt > KEY_TIME_TO_TICK( KEY_LONG_CHECK_TIME_MS ) ) {
        if( p_key->tick_cnt == KEY_TIME_TO_TICK(KEY_LONG_LONG_CHECK_TIME_MS) ) {
            // 触发超长按消息
            p_key->msg = KEY_LONG_LONG;
            #if MSG_LONG_LONG_EN
            g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_LONG_LONG );
            #endif
        } else if ( 0 == ((p_key->tick_cnt-KEY_TIME_TO_TICK(KEY_LONG_CHECK_TIME_MS))%KEY_TIME_TO_TICK(KEY_LONG_HOLD_CHECK_PER_MS)) ) {
            // 触发长按保持消息
            #if MSG_LONG_HOLD_EN
            g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_LONG_HOLD );
            #endif
        }
    } else if( p_key->tick_cnt == KEY_TIME_TO_TICK( KEY_LONG_CHECK_TIME_MS ) ) {
        // 触发长按消息
        p_key->msg = KEY_LONG;
        #if MSG_LONG_EN
        g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_LONG );
        #endif
    }
}

/**
 * @brief 按键按下处理
 * @param p_key: 按键结构体指针
 * @return void
 */
static inline void 
__key_scan_press_proc( key_scan_key_t *p_key )
{
    // 触发按下消息
    #if MSG_PRESS_EN
    g_key_scan.callback.pfn_key_trig_cb( p_key->port, KEY_PRESS );
    #endif

    //触发单击
    if( KEY_NULL == p_key->msg ) {
        p_key->msg = KEY_PRESS;
        return;
    }

    // 多击
    #if MSG_MULT_HIT_EN
    if( p_key->max_hit <= 1 ) {
        return;
    }

    if( KEY_PRESS == p_key->msg ) {
        p_key->msg = KEY_MULT_HIT_2;     //双击
    } else if( p_key->msg >= KEY_MULT_HIT_2 ) {
        if( p_key->max_hit > KEY_GET_MULT_HIT_NUM(p_key->msg) ) {
            p_key->msg++;       //实际多击在释放时处理
        } else {
            p_key->msg = KEY_ERR;
        }
    }
    #endif
}

/**
 * @brief 按键释放状态处理
 * @param p_key: 按键结构体指针
 * @param new_sta: 新状态
 * @return void
 */
static inline void 
__key_scan_proc_release( key_scan_key_t *p_key, uint8_t new_sta )
{
    if( KEY_TRIG_STA_PRESS == new_sta ) {
        // 按下消抖
        p_key->sta = KEY_STA_PRESS_CHECK;
        // 扫描标志置位，在扫描完成时清除
        if( KEY_SCAN_DISABLE == p_key->scan_flg ) {
            p_key->scan_flg = KEY_SCAN_ENABLE;
            g_key_scan.trig_num++;
            // 计数值清零
            p_key->tick_cnt = 0;

            // // 消抖tick为0，再次执行判定
            // if( 0 == KEY_TIME_TO_TICK(KEY_TRIG_FILTER_MS) ) {
            //     continue;
            // }
        }
    } else {
        if( KEY_SCAN_DISABLE == p_key->scan_flg ) {
            return;
        }
        // 保持释放处理
        __key_scan_release_hold( p_key );
    }
}

/**
 * @brief 按键按下状态处理
 * @param p_key: 按键结构体指针
 * @param new_sta: 新状态
 * @return void
 */
static inline void 
__key_scan_proc_press( key_scan_key_t *p_key, uint8_t new_sta )
{
    if( KEY_SCAN_DISABLE == p_key->scan_flg ) {
        return;
    }

    if( KEY_TRIG_STA_RELEASE == new_sta ) {
        // 释放消抖（固定一个扫描周期）
        p_key->sta = KEY_STA_RELEASE_CHECK;
    } else {
        // 按下保持处理
        __key_scan_press_hold( p_key );
    }
}

/**
 * @brief 按键释放检测处理
 * @param p_key: 按键结构体指针
 * @param new_sta: 新状态
 * @return void
 */
static inline void 
__key_scan_proc_release_check( key_scan_key_t *p_key, uint8_t new_sta )
{
    if( KEY_SCAN_DISABLE == p_key->scan_flg ) {
        return;
    }

    if( KEY_TRIG_STA_RELEASE == new_sta ) {
        p_key->sta = KEY_STA_RELEASE;
        p_key->tick_cnt = 0;
        // 释放处理
        __key_scan_release_proc( p_key );
    } else {
        // 释放为抖动，恢复状态
        p_key->sta = KEY_STA_PRESS;
        // 按下保持处理
        __key_scan_press_hold( p_key );
    }
}

/**
 * @brief 按键按下检测处理
 * @param p_key: 按键结构体指针
 * @param new_sta: 新状态
 * @return void
 */
static inline void 
__key_scan_proc_press_check( key_scan_key_t *p_key, uint8_t new_sta )
{
    if( KEY_SCAN_DISABLE == p_key->scan_flg ) {
        return;
    }

    if( KEY_TRIG_STA_PRESS == new_sta ) {
        // 按下消抖判定
        if( p_key->tick_cnt >= KEY_TIME_TO_TICK(KEY_TRIG_FILTER_MS) ) {
            p_key->sta = KEY_STA_PRESS;
            p_key->tick_cnt = 0;
            // 触发处理
            __key_scan_press_proc( p_key );
        }
    } else {
        // 按下为抖动，恢复状态
        p_key->sta = KEY_STA_RELEASE;
        // 保持释放处理
        __key_scan_release_hold( p_key );
    }
}

/**
 * @brief 按键异常处理
 * @param p_key: 按键结构体指针
 * @param new_sta: 新状态
 * @return void
 */
static inline void 
__key_scan_proc_err( key_scan_key_t *p_key, uint8_t new_sta )
{
    // 等待按键释放后解除锁定
    if( KEY_TRIG_STA_RELEASE == new_sta ) {
        p_key->sta = KEY_STA_RELEASE;
    }
}

/**
 * @brief 按键扫描处理
 * @return void
 */
void 
key_scan_scan_proc( void )
{
    // 未就绪，不进行扫描处理
    if( KEY_SCAN_READY != g_key_scan.sta_flg ) {
        return;
    }

    key_scan_key_t *l_pKey = g_key_scan.key_list;
    enum_KeyTrigSta_t l_TrigSta = KEY_TRIG_STA_RELEASE;

    for( ; l_pKey != NULL; l_pKey = l_pKey->p_next ) {
        // 按键状态获取
        l_TrigSta = g_key_scan.callback.pfn_get_key_sta( l_pKey->port );
        switch( l_pKey->sta ) {
            // 按键状态：释放
            case KEY_STA_RELEASE: {
                __key_scan_proc_release( l_pKey, l_TrigSta );
            }break;
            // 按键状态：触发
            case KEY_STA_PRESS: {
                __key_scan_proc_press( l_pKey, l_TrigSta );
            }break;

            // 按键状态：释放检测
            case KEY_STA_RELEASE_CHECK: {
                __key_scan_proc_release_check( l_pKey, l_TrigSta );
            }break;
            // 按键状态：触发检测
            case KEY_STA_PRESS_CHECK: {
                __key_scan_proc_press_check( l_pKey, l_TrigSta );
            }break;

            // 按键状态：异常
            default: {
                __key_scan_proc_err( l_pKey, l_TrigSta );
            }break;
        }

        // 计数值增加
        if( KEY_SCAN_ENABLE == l_pKey->scan_flg ) {
            l_pKey->tick_cnt++;
            //防止溢出
            if( l_pKey->tick_cnt > ( KEY_SCAN_MAX_CNT - KEY_TIME_TO_TICK(KEY_LONG_HOLD_CHECK_PER_MS) ) ) {
                l_pKey->tick_cnt -= KEY_TIME_TO_TICK(KEY_LONG_HOLD_CHECK_PER_MS);
            }
        }
    }

    #if KEY_SCAN_TRIG_EN
    if( 0 == g_key_scan.trig_num ) {
        // 无按键需要扫描，停止扫描定时器
        g_key_scan.callback.pfn_key_scan_stop( );
        KEY_SCAN_DEBUG( "key_scan_scan_proc: No Trig\r\n" );
    }
    #endif
}

#if KEY_SCAN_TRIG_EN
/**
 * @brief 按键触发扫描处理
 * @return void
 */
void 
key_scan_trig_scan( void )
{
    // 未就绪，不进行触发判断
    if( KEY_SCAN_READY != g_key_scan.sta_flg ) {
        return;
    }

    // 已经启动扫描就无需再判断
    if( g_key_scan.trig_num ) {
        return;
    }

    // 判断是否有按键触发
    key_scan_key_t *l_pKey = g_key_scan.key_list;
    for( ; NULL != l_pKey; l_pKey = l_pKey->p_next ) {
        enum_KeyTrigSta_t l_sta = g_key_scan.callback.pfn_get_key_sta( l_pKey->port );
        if( KEY_TRIG_STA_PRESS == l_sta ) {
            // 异常状态按键不触发扫描
            if( l_pKey->sta == KEY_STA_ERR ) {
                continue;
            }
            // 有按键触发，启动扫描定时器
            g_key_scan.callback.pfn_key_scan_start( KEY_SCAN_PERIOD_MS );
            // 先执行一次扫描处理
            key_scan_scan_proc( );
            KEY_SCAN_DEBUG( "key_scan_trig_scan: Trig\r\n" );
            return;
        } else {
            // 异常状态按键处理，释放后可进行触发
            if( l_pKey->sta == KEY_STA_ERR ) {
                l_pKey->sta = KEY_STA_RELEASE;
            }
        }
    }
}
#endif

/**
 * @brief 按键扫描初始化
 * @param pfn_Cb 回调函数
 * @return enum_KeyScanRet_t
 */
enum_KeyScanRet_t 
key_scan_init( key_scan_cb_t *pfn_Cb )
{
    if( NULL == pfn_Cb ) {
        return KEY_SCAN_RET_ERR_PARAM;
    }

    if( NULL == pfn_Cb->pfn_get_key_sta 
    #if KEY_SCAN_TRIG_EN
     || NULL == pfn_Cb->pfn_key_scan_start 
     || NULL == pfn_Cb->pfn_key_scan_stop 
    #endif
     || NULL == pfn_Cb->pfn_key_trig_cb ) {
        return KEY_SCAN_RET_ERR_PARAM;
    }

    //锁定
    g_key_scan.sta_flg = KEY_SCAN_BUSY;

    //设置回调函数
    g_key_scan.callback.pfn_get_key_sta = pfn_Cb->pfn_get_key_sta;
    g_key_scan.callback.pfn_key_trig_cb = pfn_Cb->pfn_key_trig_cb;

    #if KEY_SCAN_TRIG_EN
    //扫描控制函数
    g_key_scan.callback.pfn_key_scan_start = pfn_Cb->pfn_key_scan_start;
    g_key_scan.callback.pfn_key_scan_stop = pfn_Cb->pfn_key_scan_stop;
    #endif

    //按键列表相关参数初始化
    g_key_scan.key_list = NULL;
    g_key_scan.trig_num = 0;

    //初始化
    g_key_scan.sta_flg = KEY_SCAN_READY;

    return KEY_SCAN_RET_OK;
}

/**
 * @brief 按键扫描按键添加
 * @param p_key 按键结构体指针
 * @param port 按键端口号
 * @param max_hit 最大点击数
 * @return enum_KeyScanRet_t
 */
enum_KeyScanRet_t 
key_scan_key_add( key_scan_key_t *p_key, uint8_t port, uint8_t max_hit )
{
    // 未就绪，不可添加按键
    if( KEY_SCAN_READY != g_key_scan.sta_flg ) {
        return KEY_SCAN_RET_ERR_INIT;
    }

    // 按键不存在、者最大点击数为0、最大点击数超限制，返回错误
    if( NULL == p_key || 0 == max_hit || max_hit >= KEY_GET_MULT_HIT_NUM(KEY_ERR) ) {
        return KEY_SCAN_RET_ERR_PARAM;
    }

    // 锁定标志(修改期间不执行扫描)
    g_key_scan.sta_flg = KEY_SCAN_BUSY;

    // 删除再添加（防止按键重复添加）
    key_scan_key_del( p_key );

    // 初始化按键参数
    memset( p_key, 0, sizeof(key_scan_key_t) );
    p_key->port = port;
    #if MSG_MULT_HIT_EN
    p_key->max_hit = max_hit;
    #else
    (void)max_hit;
    #endif

    // 添加按键到链表头
    p_key->p_next = g_key_scan.key_list;
    g_key_scan.key_list = p_key;

    // 异常处理，防止初始化后立即触发，需要释放后才会触发
    if( KEY_TRIG_STA_PRESS == g_key_scan.callback.pfn_get_key_sta( port ) ) {
        p_key->sta = KEY_STA_ERR;
    }

    // 解锁标志
    g_key_scan.sta_flg = KEY_SCAN_READY;

    return KEY_SCAN_RET_OK;
}

/**
 * @brief 按键扫描按键删除
 * @param p_key 按键结构体指针
 * @return enum_KeyScanRet_t
 */
enum_KeyScanRet_t 
key_scan_key_del( key_scan_key_t *p_key )
{
    // 未就绪，不可删除按键
    if( KEY_SCAN_READY != g_key_scan.sta_flg ) {
        return KEY_SCAN_RET_ERR_INIT;
    }

    if( NULL == p_key ) {
        return KEY_SCAN_RET_ERR_PARAM;
    }

    // 判断是否为空
    if( NULL == g_key_scan.key_list ) {
        return KEY_SCAN_RET_ERR_NO_PORT;
    }

    enum_KeyScanRet_t ret = KEY_SCAN_RET_ERR_NO_PORT;

    // 锁定标志(修改期间不执行扫描)
    g_key_scan.sta_flg = KEY_SCAN_BUSY;

    // 删除头结点(如果需要删除的按键为头结点)
    if( g_key_scan.key_list == p_key ) {
        g_key_scan.key_list = p_key->p_next;
        ret = KEY_SCAN_RET_OK;
    } else {
        key_scan_key_t *l_pTemp = g_key_scan.key_list;
        for( ; NULL != l_pTemp; l_pTemp = l_pTemp->p_next ) {
            if( p_key == l_pTemp->p_next ) {
                l_pTemp->p_next = p_key->p_next;
                ret = KEY_SCAN_RET_OK;
                break;
            }
        }
    }

    // 解锁标志
    g_key_scan.sta_flg = KEY_SCAN_READY;

    return ret;
}
