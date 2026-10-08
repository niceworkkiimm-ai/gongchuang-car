#include "maixcam_protocol.h"

void MaixCAM_ParserReset(MaixCAM_Parser *parser)
{
    parser->count = 0;
    parser->last_byte_ms = 0;
    parser->accepted = 0;
    parser->rejected = 0;
}

static int16_t read_signed_be16(const uint8_t *bytes)
{
    uint16_t raw = (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    int32_t value = raw;
    if (raw >= 0x8000U)
        value -= 65536L;
    return (int16_t)value;
}

uint8_t MaixCAM_ParseByte(MaixCAM_Parser *parser, uint8_t byte,
                        uint32_t now_ms, MaixCAM_Target *target)
{
    uint8_t i, sum = 0;
    if (parser->count &&
        (uint32_t)(now_ms - parser->last_byte_ms) > MAIXCAM_INTERBYTE_TIMEOUT_MS)
        parser->count = 0;
    parser->last_byte_ms = now_ms;
    if (parser->count == 0 && byte != 0xAA)
        return 0;
    parser->bytes[parser->count++] = byte;
    if (parser->count < MAIXCAM_FRAME_SIZE)
        return 0;

    for (i = 1; i <= 5; ++i)
        sum ^= parser->bytes[i];
    if (parser->bytes[0] == 0xAA && parser->bytes[1] <= 1 &&
        parser->bytes[6] == sum && parser->bytes[7] == 0x55)
    {
        target->found = parser->bytes[1];
        target->dx = target->found ? read_signed_be16(&parser->bytes[2]) : 0;
        target->dy = target->found ? read_signed_be16(&parser->bytes[4]) : 0;
        ++target->packet_count;
        target->received_ms = now_ms;
        ++parser->accepted;
        parser->count = 0;
        return 1;
    }

    ++parser->rejected;
    /* 固定长度滑窗重同步；数据内部出现 AA/55 不会提前截断帧。 */
    for (i = 1; i < MAIXCAM_FRAME_SIZE; ++i)
        if (parser->bytes[i] == 0xAA)
            break;
    parser->count = (uint8_t)(MAIXCAM_FRAME_SIZE - i);
    if (parser->count)
    {
        uint8_t j;
        for (j = 0; j < parser->count; ++j)
            parser->bytes[j] = parser->bytes[i + j];
    }
    return 0;
}

uint8_t MaixCAM_TargetFresh(const MaixCAM_Target *target, uint32_t now_ms,
                          uint32_t timeout_ms)
{
    return (uint8_t)(target->found &&
        (uint32_t)(now_ms - target->received_ms) <= timeout_ms);
}
