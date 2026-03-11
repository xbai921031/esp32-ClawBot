/*
 *    Autor: xbai
 *    Date: 2026/01/18
 */

#include "os_alarm.h"
#include "esp_rom_sys.h"

static void AlarmCallback(void* arg)
{
    Os_Alarm_t* alarm = (Os_Alarm_t*)arg;

    BaseType_t hpw = pdFALSE;
    vTaskNotifyGiveFromISR(*(alarm->task), &hpw);

    if(hpw)
    {
        portYIELD_FROM_ISR();
    }

    /* Change to period task */
    if(alarm->period_us > 0)
    {
        esp_timer_stop(alarm->timer);
        esp_timer_start_periodic(alarm->timer, alarm->period_us);
        alarm->period_us = 0;
    }
}

void Os_AlarmInit(void)
{
    for(uint32_t i = 0; i < OS_ALARM_NUM; i++)
    {
        esp_timer_create_args_t args = {
            .callback = AlarmCallback,
            .arg = &OsAlarmTable[i],
            .name = "OsAlarm"};

        esp_timer_create(&args, &OsAlarmTable[i].timer);

        /* Offset start */
        esp_timer_start_once(OsAlarmTable[i].timer, OsAlarmTable[i].offset_us);
    }

    esp_rom_printf("OS Alarms started (offset + period)\r\n");
}
