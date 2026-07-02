/*
 *    Autor: xbai
 *    Date: 2026/01/18
 */

#include "Os.h"
#include "Os_Alarm.h"
#include "esp_rom_sys.h"
#include "agent_loop.h"

TaskHandle_t OsTask1Handle;
TaskHandle_t OsTask2Handle;

StaticTask_t OsTask1TCB;
StaticTask_t OsTask2TCB;

StackType_t OsTask1Stack[8192];
StackType_t OsTask2Stack[8192];

/* Agent task */
static void OsTask1(void* arg)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        agent_loop();
        //esp_rom_printf("[Task1] time=%llu\r\n", esp_timer_get_time());
    }
}

/**/
static void OsTask2(void* arg)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        //esp_rom_printf("[Task2] time=%llu\r\n", esp_timer_get_time());
    }
}

void OS_Init(void)
{
    OsTask1Handle = xTaskCreateStatic(
        OsTask1,
        "OsTask1",
        8192,
        NULL,
        4,
        OsTask1Stack,
        &OsTask1TCB);

    OsTask2Handle = xTaskCreateStatic(
        OsTask2,
        "OsTask2",
        8192,
        NULL,
        3,
        OsTask2Stack,
        &OsTask2TCB);
}

void OS_Start(void)
{
    Os_AlarmInit();
}
