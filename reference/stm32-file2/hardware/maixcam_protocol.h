#ifndef MAIXCAM_PROTOCOL_H
#define MAIXCAM_PROTOCOL_H
#include <stdint.h>

#define MAIXCAM_FRAME_SIZE 8U
#define MAIXCAM_INTERBYTE_TIMEOUT_MS 50U

typedef enum {
    MAIXCAM_MODE_OFF = 0,
    MAIXCAM_MODE_RED = 1,
    MAIXCAM_MODE_YELLOW = 2,
    MAIXCAM_MODE_BLUE = 3,
    MAIXCAM_MODE_GREEN = 4,
    MAIXCAM_MODE_BLACK = 5,
    MAIXCAM_MODE_LIGHT_BLUE = 6,
    MAIXCAM_MODE_RING = 7
} MaixCAM_Mode;

typedef struct {
    uint8_t found;
    int16_t dx;
    int16_t dy;
    uint32_t packet_count;
    uint32_t received_ms;
} MaixCAM_Target;

typedef struct {
    uint8_t bytes[MAIXCAM_FRAME_SIZE];
    uint8_t count;
    uint32_t last_byte_ms;
    uint32_t accepted;
    uint32_t rejected;
} MaixCAM_Parser;

void MaixCAM_ParserReset(MaixCAM_Parser *parser);
uint8_t MaixCAM_ParseByte(MaixCAM_Parser *parser, uint8_t byte,
                        uint32_t now_ms, MaixCAM_Target *target);
uint8_t MaixCAM_TargetFresh(const MaixCAM_Target *target, uint32_t now_ms,
                          uint32_t timeout_ms);
#endif
