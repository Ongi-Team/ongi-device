#include "rtc_driver.h"
#include "rtc_internal.h"

static const RtcDriver INTERNAL_RTC_DRIVER = {
    .init = internal_rtc_init,
    .sync = internal_rtc_sync,
    .get_time = internal_rtc_get_time,
    .is_synced = internal_rtc_is_synced
};

const RtcDriver *rtc_get_driver(void) {
    return &INTERNAL_RTC_DRIVER;
}