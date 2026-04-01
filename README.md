# Mods-keyscan
按键扫描模块  

- [简介](#简介)  
- [配置项](#配置项)
  - [扫描配置](#扫描配置)
  - [消息配置](#消息配置)
  - [时间配置](#时间配置)
- [函数接口](#函数接口)
  - [按键扫描初始化函数](#按键扫描初始化函数)
  - [按键扫描执行函数](#按键扫描执行函数)
  - [按键扫描触发处理函数](#按键扫描触发处理函数)
  - [添加按键函数](#添加按键函数)
  - [删除按键函数](#删除按键函数)


## 简介
基于按键链表实现的按键扫描模块，支持动态扫描控制。

支持下列消息  

| 消息 | 描述 | 触发条件 |
| --- | --- | --- |
| KEY_PRESS | 按下 | 按下按键立即触发 |
| KEY_RELEASE | 抬起 | 抬起按键立即触发 |
| KEY_SHORT_UP | 短按抬起 | 单击按键，且按下时间小于 `KEY_LONG_CHECK_TIME_MS` 时触发 |
| KEY_LONG | 长按 | 按下按键，且按下时间大于 `KEY_LONG_CHECK_TIME_MS` 时触发 |
| KEY_LONG_HOLD | 长按保持 | 触发长按后继续保持按下状态，周期性触发 |
| KEY_LONG_LONG | 超长按 | 按下按键，且按下时间大于 `KEY_LONG_LONG_CHECK_TIME_MS` 时触发 |
| KEY_LONG_UP | 长按抬起 | 触发长按后抬起按键触发 |
| KEY_MULT_HIT | 多击 | 按键连续点击，且点击间隔小于 `KEY_MULT_HIT_CHECK_TIME_MS` 时触发 |

## 配置项

### 扫描配置

`KEY_SCAN_TRIG_EN` : 是否使能扫描触发  
`0` ：失能。直接周期性调用`key_scan_scan_proc`进行扫描。  
`1` ：使能。按需启用按键扫描，当有按键按下后启用扫描，所有按键释放后停止扫描。  
使用`key_scan_trig_scan`函数检测是否有有按键按下，检测到有按键按下后使用`pfn_key_scan_start`启动扫描，当所有按键释放后使用`pfn_key_scan_stop`停止扫描。`key_scan_scan_proc`函数在受控定时器中调用。  

### 消息配置
通过下列宏可选择使用的消息  

| 宏名称 | 功能 |
| --- | --- |
| MSG_PRESS_EN | 按下消息使能 |
| MSG_RELEASE_EN | 释放消息使能 |
| MSG_SHORT_UP_EN | 短按抬起消息使能 |
| MSG_LONG_EN | 长按消息使能 |
| MSG_LONG_LONG_EN | 超长按消息使能 |
| MSG_LONG_HOLD_EN | 长按保持消息使能 |
| MSG_LONG_UP_EN | 长按抬起消息使能 |
| MSG_MULT_HIT_EN | 多击消息使能 （使能时会降低 MSG_SHORT_UP_EN 的响应速度） |


### 时间配置
通过下列宏配置按键扫描相关的时间参数

| 宏名称 | 功能 | 默认值 |
| --- | --- | --- |
| KEY_SCAN_PERIOD_MS | 按键扫描周期 | 10 |
| KEY_TRIG_FILTER_MS | 按键按下消抖时间 | 10 |
| KEY_LONG_CHECK_TIME_MS | 长按触发时间 | 1000 |
| KEY_LONG_LONG_CHECK_TIME_MS | 超长按触发时间 | 3000 |
| KEY_LONG_HOLD_CHECK_PER_MS | 长按保持触发周期时间 | 500 |
| KEY_MULT_HIT_CHECK_TIME_MS | 多击检测时间 | 200 |


## 函数接口
### 按键扫描初始化函数

``` c
/**
 * @brief 按键扫描初始化
 * @param pfn_Cb 回调函数
 * @return enum_KeyScanRet_t
 */
enum_KeyScanRet_t 
key_scan_init( key_scan_cb_t *pfn_Cb );
```
使用按键扫描前必须先执行的函数，初始化按键扫描模块，需提供有效回调，否则初始化失败。  

`pfn_get_key_sta` 为获取按键状态的函数，返回按键状态。  
`pfn_key_trig_cb` 为按键触发回调，返回按键触发状态。  
`pfn_key_scan_start` 为按键扫描启动函数，`KEY_SCAN_TRIG_EN`使能时有效。  
`pfn_key_scan_stop` 为按键扫描停止函数，`KEY_SCAN_TRIG_EN`使能时有效。  

### 按键扫描执行函数

``` c
/**
 * @brief 按键扫描处理
 * @return void
 */
void 
key_scan_scan_proc( void );
```
按键扫描处理函数，必须每 `KEY_SCAN_PERIOD_MS` 毫秒调用一次。

### 按键扫描触发处理函数

``` c
/**
 * @brief 按键触发扫描处理
 * @return void
 */
void 
key_scan_trig_scan( void );
```
按键触发扫描处理函数，当 `KEY_SCAN_TRIG_EN` 使能后有效，用于按键触发后启动扫描。

### 添加按键函数

``` c
/**
 * @brief 按键扫描按键添加
 * @param p_key 按键结构体指针
 * @param port 按键端口号
 * @param max_hit 最大点击数
 * @return enum_KeyScanRet_t
 */
enum_KeyScanRet_t 
key_scan_key_add( key_scan_key_t *p_key, uint8_t port, uint8_t max_hit );
```
添加按键函数，添加按键到按键扫描列表。

### 删除按键函数

``` c
/**
 * @brief 按键扫描按键删除
 * @param p_key 按键结构体指针
 * @return enum_KeyScanRet_t
 */
enum_KeyScanRet_t 
key_scan_key_del( key_scan_key_t *p_key );
```
删除按键函数，从按键扫描列表中删除按键。
