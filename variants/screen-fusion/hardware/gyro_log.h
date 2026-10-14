#ifndef GYRO_LOG_H
#define GYRO_LOG_H

#include "stm32f10x.h"

/* Set to 0 and rebuild to remove all logging hooks and RAM buffers. */
#ifndef GYRO_LOG_ENABLE
#define GYRO_LOG_ENABLE 1
#endif
#ifndef GYRO_LOG_EVENT_CAPACITY
#define GYRO_LOG_EVENT_CAPACITY 512U
#endif
#ifndef GYRO_LOG_SAMPLE_CAPACITY
#define GYRO_LOG_SAMPLE_CAPACITY 1024U
#endif
#define GYRO_LOG_FAST_US 20000UL
#define GYRO_LOG_SLOW_US 200000UL
#define GYRO_LOG_TAIL_US 500000UL

enum {
    GLOG_BOOT = 1, GLOG_CAR_BEGIN, GLOG_VELOCITY_SENT,
    GLOG_HEADING_WINDOW, GLOG_STOP_SENT, GLOG_CAR_END,
    GLOG_POST_50MS, GLOG_POST_200MS, GLOG_POST_500MS,
    GLOG_TASK_COMPLETE, GLOG_FROZEN,
    GLOG_SAMPLE = 100
};

/* Exact wire/dump layout: 32 bytes, little endian, IEEE754 float32.
 * SENT means the original send function returned, NOT a motor ACK.
 * direction bit 0 is the sent direction; bit 7 means car() is active.
 * The target is the last car(0) target; meaningful together with event/mode.
 */
typedef struct {
    uint32_t time_us;
    float yaw_deg;
    float control_deg;
    float rate_dps;
    uint32_t yaw_frames;
    uint32_t last_yaw_us;
    int16_t target_deg;
    int16_t mode;
    uint16_t event;
    uint8_t motor;
    uint8_t direction;
} GyroLogRecord;

typedef struct {
    uint32_t magic, version, header_size, record_size;
    uint32_t event_capacity, sample_capacity;
    uint32_t event_count, sample_count, sample_next, sample_total;
    uint32_t events_dropped, frozen, clock_hz;
    uint32_t max_write_cycles, max_tick_cycles, flags;
} GyroLogHeader;

typedef struct {
    GyroLogHeader header;
    GyroLogRecord events[GYRO_LOG_EVENT_CAPACITY];
    GyroLogRecord samples[GYRO_LOG_SAMPLE_CAPACITY];
} GyroLogStore;

#if GYRO_LOG_ENABLE
extern volatile GyroLogStore gyro_log;
void GyroLog_Init(void);
void GyroLog_OnYaw(void);
void GyroLog_Tick(void);
void GyroLog_CarBegin(int mode, int target);
void GyroLog_CarEnd(void);
void GyroLog_Mark(uint16_t event, uint8_t motor, uint8_t direction);
void GyroLog_TaskComplete(void);
#else
#define GyroLog_Init() ((void)0)
#define GyroLog_OnYaw() ((void)0)
#define GyroLog_Tick() ((void)0)
#define GyroLog_CarBegin(mode, target) ((void)0)
#define GyroLog_CarEnd() ((void)0)
#define GyroLog_Mark(event, motor, direction) ((void)0)
#define GyroLog_TaskComplete() ((void)0)
#endif
#endif
