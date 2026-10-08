#include "openmv.h"
#include "Emm_V5.h"
#include "delay.h"

volatile VisionResult vision_last_result = VISION_OK;

static void vision_stop(void)
{
    uint8_t id;
    for (id = 1; id <= 4; ++id)
    {
        Emm_V5_Stop_Now(id, 0);
        delay_ms(10);
    }
}

static VisionResult finish(VisionResult result)
{
    vision_stop();
    MaixCAM_SetMode(MAIXCAM_MODE_OFF);
    vision_last_result = result;
    return result;
}

static void move_axis(uint8_t y_axis, int32_t error)
{
    uint8_t id, dir;
    uint16_t rpm = y_axis ? 7U : 5U;
    /* 沿用学长视觉微调的轮号、方向组合及低速参数。 */
    for (id = 1; id <= 4; ++id)
    {
        if (y_axis)
            dir = (id >= 3) ? 1U : 0U;
        else
            dir = (id == 2 || id == 4) ? 1U : 0U;
        if (error < 0)
            dir ^= 1U;
        Emm_V5_Vel_Control(id, dir, rpm, (uint8_t)rpm, 0);
        delay_ms(10);
    }
}

VisionResult Vision_Align(uint8_t mode, uint16_t tolerance_px)
{
    MaixCAM_Target target;
    uint32_t started, last_packet = 0;
    uint8_t have_packet = 0, confirmed = 0;
    int32_t ex, ey;

    if (mode < MAIXCAM_MODE_RED || mode > MAIXCAM_MODE_RING)
        return finish(VISION_BAD_MODE);
    vision_stop();
    MaixCAM_SetMode(mode);
    started = MaixCAM_NowMs();

    for (;;)
    {
        uint32_t now = MaixCAM_NowMs();
        if ((uint32_t)(now - started) >= MAIXCAM_ALIGN_TIMEOUT_MS)
            return finish(VISION_ALIGN_TIMEOUT);
        if (!MaixCAM_GetTarget(&target))
        {
            /* UART 错误也清除 received，已经运动时立即终止。 */
            if (have_packet || (uint32_t)(now - started) > MAIXCAM_LINK_TIMEOUT_MS)
                return finish(VISION_LINK_TIMEOUT);
            delay_ms(10);
            continue;
        }
        if (!target.found)
            return finish(VISION_NO_TARGET);
        now = MaixCAM_NowMs();
        if (!MaixCAM_TargetFresh(&target, now, MAIXCAM_LINK_TIMEOUT_MS))
            return finish(VISION_LINK_TIMEOUT);
        if (have_packet && target.packet_count == last_packet)
        {
            delay_ms(10);
            continue;
        }
        have_packet = 1;
        last_packet = target.packet_count;
        ex = -(int32_t)target.dx;
        ey = -(int32_t)target.dy;

        if (ex <= (int32_t)tolerance_px && ex >= -(int32_t)tolerance_px &&
            ey <= (int32_t)tolerance_px && ey >= -(int32_t)tolerance_px)
        {
            vision_stop();
            if (++confirmed >= MAIXCAM_ALIGN_CONFIRM_FRAMES)
                return finish(VISION_OK);
        }
        else
        {
            confirmed = 0;
            /* 每个新包只修正一个轴，防止两个轴的命令互相覆盖。 */
            if (ex > (int32_t)tolerance_px || ex < -(int32_t)tolerance_px)
                move_axis(0, ex);
            else
                move_axis(1, ey);
        }
    }
}

VisionResult vision(uint8_t mode) { return Vision_Align(mode, 2); }
VisionResult WL_dingwei(uint8_t mode)
{
    if (mode < MAIXCAM_MODE_RED || mode > MAIXCAM_MODE_LIGHT_BLUE)
        return finish(VISION_BAD_MODE);
    return Vision_Align(mode, 5);
}
VisionResult WL_dingwei2(uint8_t mode)
{
    if (mode < MAIXCAM_MODE_RED || mode > MAIXCAM_MODE_LIGHT_BLUE)
        return finish(VISION_BAD_MODE);
    return Vision_Align(mode, 8);
}
