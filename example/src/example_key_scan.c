
#include "example_key_scan.h"


#define DEV_KEY_DEBUG_EN            1

#if DEV_KEY_DEBUG_EN
#define DEV_KEY_DEBUG               printf
#else
#define DEV_KEY_DEBUG(...)
#endif


//测试：是否使用触发检测（按需开始/停止扫描）
#define KEY_TEST_TRIG_SCAN_EN       KEY_SCAN_TRIG_EN


#if DEV_KEY_DEBUG_EN
static const char *g_key_msg_str[] = {
    "KEY_MSG_NULL",
    "KEY_PRESS",
    "KEY_RELEASE",
    "KEY_SHORT_UP",
    "KEY_LONG",
    "KEY_LONG_HOLD",
    "KEY_LONG_LONG",
    "KEY_LONG_UP",
};
#endif


static key_scan_key_t g_key_port_0;
static key_scan_key_t g_key_port_1;



#if KEY_TEST_TRIG_SCAN_EN

/**
 * @brief 按键扫描定时器处理
 */
static void 
__key_scan_timer_proc( void )
{
    key_scan_scan_proc();
}

/**
 * @brief 按键扫描定时器启动
 * @param PerMs 定时器周期，单位毫秒
 */
static void
__key_scan_start( uint32_t PerMs )
{
    
}

/**
 * @brief 按键扫描定时器停止
 */
static void 
__key_scan_stop( void )
{
    
}
#endif


/**
 * @brief 按键状态获取
 */
static enum_KeyTrigSta_t 
__get_key_sta(uint8_t Port )
{
    switch( Port ) {
        case 0: {
            return (0 != HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_10 ))?KEY_TRIG_STA_RELEASE:KEY_TRIG_STA_PRESS;
        }
        case 1: {
            return (0 != HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_11 ))?KEY_TRIG_STA_RELEASE:KEY_TRIG_STA_PRESS;
        }

        default:
            break;
    }
    return KEY_TRIG_STA_RELEASE;
}

/**
 * @brief 按键消息触发回调
 */
static void 
__key_trig_cb( uint8_t Port, uint8_t Msg )
{
    uint16_t l_msg = KEY_MSG(Port,Msg);

    //
    (void)l_msg;

    #if DEV_KEY_DEBUG_EN
    if( Msg < sizeof(g_key_msg_str)/sizeof(g_key_msg_str[0]) ) {
        DEV_KEY_DEBUG( "Key Port:%d Msg: %s\r\n", Port, g_key_msg_str[Msg] );
    } else {
        DEV_KEY_DEBUG( "Key Port:%d Msg: KEY_MULT_HIT %d\r\n", Port, Msg+2-KEY_MULT_HIT_2 );
    }
    #endif
}


/**
 * @brief 键盘扫描
 */
void 
example_key_trig_proc( void )
{
    #if KEY_TEST_TRIG_SCAN_EN
    key_scan_trig_scan();
    #else
    static uint32_t l_tick_ms = 0;
    uint32_t l_new_tick_ms = HAL_GetTick();

    if( l_new_tick_ms - l_tick_ms >= KEY_SCAN_PERIOD_MS ) {
        l_tick_ms = l_new_tick_ms;
        key_scan_scan_proc();
    }
    #endif
}

/**
 * @brief 按键扫描初始化
 */
uint8_t
example_key_scan_init( void )
{
    key_scan_cb_t l_Cb = {
        .pfn_get_key_sta = __get_key_sta,
        .pfn_key_trig_cb = __key_trig_cb,
        #if KEY_TEST_TRIG_SCAN_EN
        .pfn_key_scan_start = __key_scan_start,
        .pfn_key_scan_stop = __key_scan_stop,
        #endif
    };
    //按键扫描配置初始化
    key_scan_init( &l_Cb );

    //按键添加
    key_scan_key_add( &g_key_port_0, 0, 1 );
    key_scan_key_add( &g_key_port_1, 1, 3 );

    DEV_KEY_DEBUG("Key Scan Inited\r\n");

    return 0;
}

