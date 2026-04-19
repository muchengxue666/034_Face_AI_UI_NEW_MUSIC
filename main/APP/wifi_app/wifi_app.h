#ifndef __WIFI_APP_H
#define __WIFI_APP_H

#include <stdbool.h>

void wifi_sta_init(void);
bool wifi_is_connected(void);   /* WiFi已获取IP返回true */

#endif
