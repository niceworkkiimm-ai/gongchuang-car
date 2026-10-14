#include "gyro_log.h"

#if GYRO_LOG_ENABLE
extern volatile float global_angle, anglea, angular_velocity_z;
extern volatile uint32_t gyro_yaw_frame_count;

/* All shared access is short and bounded. No allocation, formatting, UART,
 * flash writes, motor calls or delay calls are permitted in this module.
 */
#ifndef GLOG_DWT_CTRL
#define GLOG_DWT_CTRL (*(volatile uint32_t *)0xE0001000UL)
#endif
#ifndef GLOG_DWT_CYCCNT
#define GLOG_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)
#endif
volatile GyroLogStore gyro_log;
typedef char RecordSizeMustBe32[(sizeof(GyroLogRecord) == 32U) ? 1 : -1];
typedef char HeaderSizeMustBe64[(sizeof(GyroLogHeader) == 64U) ? 1 : -1];
typedef char CapacitiesMustBePositive[
    (GYRO_LOG_EVENT_CAPACITY > 0 && GYRO_LOG_SAMPLE_CAPACITY > 0) ? 1 : -1];

static uint8_t initialized, active, tail_pending, post_mask, completing;
static int16_t current_mode = -1, last_target;
static uint32_t last_cycles, time_us, cycle_remainder, cycles_per_us;
static uint32_t last_yaw_us, last_sample_us, end_us, complete_us;

/* Called at least by the existing TIM2 tick. Unsigned subtraction supports
 * DWT wrap (~59.65 s at 72 MHz); the shared DWT counter is never reset.
 * Log microseconds wrap after ~71.6 minutes; the reader unwraps event order.
 */
static uint32_t LogClock(void)
{
    uint32_t now = GLOG_DWT_CYCCNT;
    uint32_t delta = now - last_cycles;
    last_cycles = now;
    time_us += delta / cycles_per_us;
    cycle_remainder += delta % cycles_per_us;
    time_us += cycle_remainder / cycles_per_us;
    cycle_remainder %= cycles_per_us;
    return time_us;
}

/* Caller holds PRIMASK. Publish counters only after a complete record. */
static void WriteRecord(uint16_t event, uint8_t motor, uint8_t direction,
                        uint32_t now, uint8_t sample)
{
    volatile GyroLogRecord *record;
    uint32_t start = GLOG_DWT_CYCCNT;
    uint32_t index, elapsed;
    if (sample)
        index = gyro_log.header.sample_next;
    else
    {
        index = gyro_log.header.event_count;
        if (index >= GYRO_LOG_EVENT_CAPACITY)
        {
            if (gyro_log.header.events_dropped != 0xFFFFFFFFUL)
                ++gyro_log.header.events_dropped;
            return;
        }
    }
    record = sample ? &gyro_log.samples[index] : &gyro_log.events[index];
    record->time_us = now;
    record->yaw_deg = global_angle;
    record->control_deg = anglea;
    record->rate_dps = angular_velocity_z;
    record->yaw_frames = gyro_yaw_frame_count;
    record->last_yaw_us = last_yaw_us;
    record->target_deg = last_target;
    record->mode = current_mode;
    record->event = event;
    record->motor = motor;
    record->direction = (uint8_t)((direction & 1U) | (active ? 0x80U : 0U));
    __DMB();
    if (sample)
    {
        gyro_log.header.sample_next = (index + 1U) % GYRO_LOG_SAMPLE_CAPACITY;
        if (gyro_log.header.sample_count < GYRO_LOG_SAMPLE_CAPACITY)
            ++gyro_log.header.sample_count;
        ++gyro_log.header.sample_total;
    }
    else
        gyro_log.header.event_count = index + 1U;
    elapsed = GLOG_DWT_CYCCNT - start;
    if (elapsed > gyro_log.header.max_write_cycles)
        gyro_log.header.max_write_cycles = elapsed;
}

void GyroLog_Init(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (!initialized)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        GLOG_DWT_CTRL |= 1UL;
        last_cycles = GLOG_DWT_CYCCNT;
        cycles_per_us = SystemCoreClock / 1000000UL;
        if (!cycles_per_us) cycles_per_us = 1;
        gyro_log.header.magic = 0x474C5947UL; /* bytes GYLG */
        gyro_log.header.version = 1;
        gyro_log.header.header_size = sizeof(GyroLogHeader);
        gyro_log.header.record_size = sizeof(GyroLogRecord);
        gyro_log.header.event_capacity = GYRO_LOG_EVENT_CAPACITY;
        gyro_log.header.sample_capacity = GYRO_LOG_SAMPLE_CAPACITY;
        gyro_log.header.clock_hz = SystemCoreClock;
        initialized = 1;
        WriteRecord(GLOG_BOOT, 0, 0, 0, 0);
    }
    __set_PRIMASK(mask);
}

void GyroLog_OnYaw(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (initialized && !gyro_log.header.frozen)
        last_yaw_us = LogClock();
    __set_PRIMASK(mask);
}

void GyroLog_Mark(uint16_t event, uint8_t motor, uint8_t direction)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (initialized && !gyro_log.header.frozen)
        WriteRecord(event, motor, direction, LogClock(), 0);
    __set_PRIMASK(mask);
}

void GyroLog_CarBegin(int mode, int target)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (initialized && !gyro_log.header.frozen)
    {
        current_mode = (int16_t)mode;
        active = 1;
        if (mode == 0)
        {
            last_target = (int16_t)target;
            tail_pending = 0;
        }
        WriteRecord(GLOG_CAR_BEGIN, 0, 0, LogClock(), 0);
    }
    __set_PRIMASK(mask);
}

void GyroLog_CarEnd(void)
{
    uint32_t now, mask = __get_PRIMASK();
    __disable_irq();
    if (initialized && !gyro_log.header.frozen)
    {
        now = LogClock();
        active = 0;
        WriteRecord(GLOG_CAR_END, 0, 0, now, 0);
        if (current_mode == 0)
        {
            end_us = now;
            tail_pending = 1;
            post_mask = 0;
        }
    }
    __set_PRIMASK(mask);
}

void GyroLog_TaskComplete(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (initialized && !gyro_log.header.frozen && !completing)
    {
        complete_us = LogClock();
        completing = 1;
        gyro_log.header.flags |= 1U;
        WriteRecord(GLOG_TASK_COMPLETE, 0, 0, complete_us, 0);
    }
    __set_PRIMASK(mask);
}

void GyroLog_Tick(void)
{
    uint32_t now, age, period, elapsed, start;
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    start = GLOG_DWT_CYCCNT;
    if (initialized && !gyro_log.header.frozen)
    {
        now = LogClock();
        if (tail_pending)
        {
            age = now - end_us;
            if (!(post_mask & 1U) && age >= 50000UL)
            {
                WriteRecord(GLOG_POST_50MS, 0, 0, now, 0);
                post_mask |= 1U;
            }
            if (!(post_mask & 2U) && age >= 200000UL)
            {
                WriteRecord(GLOG_POST_200MS, 0, 0, now, 0);
                post_mask |= 2U;
            }
            if (!(post_mask & 4U) && age >= GYRO_LOG_TAIL_US)
            {
                WriteRecord(GLOG_POST_500MS, 0, 0, now, 0);
                post_mask |= 4U;
                tail_pending = 0;
            }
        }
        period = ((active && current_mode == 0) || tail_pending || completing)
                 ? GYRO_LOG_FAST_US : GYRO_LOG_SLOW_US;
        if ((uint32_t)(now - last_sample_us) >= period)
        {
            WriteRecord(GLOG_SAMPLE, 0, 0, now, 1);
            last_sample_us = now;
        }
        if (completing && (uint32_t)(now - complete_us) >= GYRO_LOG_TAIL_US)
        {
            WriteRecord(GLOG_FROZEN, 0, 0, now, 0);
            gyro_log.header.frozen = 1;
        }
        elapsed = GLOG_DWT_CYCCNT - start;
        if (elapsed > gyro_log.header.max_tick_cycles)
            gyro_log.header.max_tick_cycles = elapsed;
    }
    __set_PRIMASK(mask);
}
#endif
