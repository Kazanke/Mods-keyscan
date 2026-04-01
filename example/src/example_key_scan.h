
#ifndef __EXAMPLE_KEY_SCAN_H__
#define __EXAMPLE_KEY_SCAN_H__

#include "key_scan.h"


#define KEY_MSG_NULL            ((uint16_t)0)

#define KEY_MSG(Port,Msg)       ((uint16_t)(((Port)<<8)|((Msg)&0xFF)))


uint8_t
example_key_scan_init( void );

void 
example_key_trig_proc( void );

#endif
