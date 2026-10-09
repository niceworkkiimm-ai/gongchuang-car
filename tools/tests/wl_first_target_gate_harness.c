#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static uint32_t now_ms, fake_ctrl, fake_cycles;
static uint32_t camera_debug_frames, camera_debug_last_frame_ms;
static uint8_t maixcam_found;
static int x, y, error_x, error_y, pos_x, pos_y, flag_n, flag_a;
static uint8_t rxCmd[4];
static int scanner_stage, UART4;
static uint32_t SystemCoreClock = 1000000U;
static struct { uint32_t DEMCR; } fake_core;
#define CoreDebug (&fake_core)
#define CoreDebug_DEMCR_TRCENA_Msk 1U
static uint32_t __get_PRIMASK(void) { return 0; }
static void __disable_irq(void) {}
static void __set_PRIMASK(uint32_t value) { (void)value; }
static void __DSB(void) {}
static void __ISB(void) {}
static uint32_t CameraDebug_NowMs(void) { return now_ms; }
static void WheelReply_End(void) {}
static void WheelReply_Begin(void) {}
static void WheelReply_Arm(uint8_t addr) { (void)addr; }
static unsigned stops, moves, commands;
static uint32_t first_move_ms;
static void Emm_V5_Stop_Now(uint8_t addr, int sync)
{ (void)addr; (void)sync; ++stops; }
static void Emm_V5_Pos_Control(uint8_t addr, uint8_t dir, int speed, int acc,
                               uint32_t pulses, int mode, int sync)
{
    (void)addr; (void)dir; (void)speed; (void)acc;
    (void)pulses; (void)mode; (void)sync;
    if (!moves++) first_move_ms = now_ms;
}
static void Usart_SendByte4(int port, char mode)
{ (void)port; assert(mode == '1'); ++commands; }
static uint32_t events[8];
static unsigned event_count, event_index;
static void frame(uint32_t received_at, int found, int ix, int iy)
{
    ++camera_debug_frames;
    camera_debug_last_frame_ms = received_at;
    maixcam_found = (uint8_t)found;
    x = ix; y = iy; error_x = 155 - ix; error_y = 125 - iy;
}
static void delay_ms(uint32_t ms)
{
    while (ms--) {
        ++now_ms; fake_cycles += SystemCoreClock / 1000U;
        assert(now_ms < 30000U); /* Detect an unexpected infinite wait. */
        if (event_index < event_count && now_ms == events[event_index]) {
            frame(now_ms, 1, 150, 100); ++event_index;
        }
    }
}

#include "locator_under_test.c"

static void reset(void)
{
    now_ms = fake_cycles = fake_ctrl = 0;
    camera_debug_frames = camera_debug_last_frame_ms = 0;
    maixcam_found = 0; x = y = error_x = error_y = 0;
    event_count = event_index = stops = moves = commands = 0;
    first_move_ms = 0;
}

static void gate_cases(void)
{
    uint32_t seen = 0, start = 10000U;
    uint8_t skip = 0;
    reset(); now_ms = start;
    assert(!WL_FirstTargetReady(start, &seen, &skip)); /* No cached frame. */
    frame(start - 1U, 1, 155, 125);
    assert(!WL_FirstTargetReady(start, &seen, &skip) && !skip);
    now_ms = start + 500U; frame(now_ms, 0, 155, 125);
    assert(!WL_FirstTargetReady(start, &seen, &skip) && !skip);
    frame(now_ms, 1, 320, 125);
    assert(!WL_FirstTargetReady(start, &seen, &skip) && !skip);
    frame(now_ms, 1, 155, 0);
    assert(!WL_FirstTargetReady(start, &seen, &skip) && !skip);
    frame(now_ms, 1, 155, 125);
    assert(!WL_FirstTargetReady(start, &seen, &skip) && skip);
    now_ms = start + 2500U; frame(now_ms, 1, 155, 125);
    assert(!WL_FirstTargetReady(start, &seen, &skip));
    frame(start + 3999U, 1, 155, 125); now_ms = start + 4000U;
    assert(!WL_FirstTargetReady(start, &seen, &skip)); /* Arrival time matters. */
    now_ms = start + 4100U;
    assert(!WL_FirstTargetReady(start, &seen, &skip)); /* Cannot reuse that frame. */
    frame(start + 4000U, 1, 155, 125);
    assert(WL_FirstTargetReady(start, &seen, &skip));
    assert(!WL_FirstTargetReady(start, &seen, &skip));

    skip = 0; now_ms = start + 2000U; frame(now_ms, 1, 155, 125);
    assert(!WL_FirstTargetReady(start, &seen, &skip) && skip);
    skip = 0; now_ms = start + 2001U; frame(now_ms, 1, 155, 125);
    assert(WL_FirstTargetReady(start, &seen, &skip) && !skip);
    now_ms = start + 5000U; frame(now_ms - VISION_FRAME_STALE_MS, 1, 155, 125);
    assert(!WL_FirstTargetReady(start, &seen, &skip));

    /* Both elapsed time and the frame sequence can wrap at 32 bits. */
    start = UINT32_MAX - 1000U; skip = 0;
    camera_debug_frames = UINT32_MAX; seen = camera_debug_frames;
    now_ms = start + 500U; frame(now_ms, 1, 155, 125);
    assert(!WL_FirstTargetReady(start, &seen, &skip) && skip);
    now_ms = start + 4000U; frame(now_ms, 1, 155, 125);
    assert(WL_FirstTargetReady(start, &seen, &skip));
}

static void integration(uint32_t first, uint32_t accepted, int early)
{
    reset();
    events[event_count++] = first;
    if (early) {
        events[event_count++] = 2500U;
        events[event_count++] = 3999U;
        events[event_count++] = accepted;
    }
    /* Keep providing valid data throughout the existing alignment window. */
    events[event_count++] = accepted + 500U;
    events[event_count++] = accepted + 1000U;
    WL_dingwei('1');
    assert(commands == 1 && stops == 4 && moves > 0);
    assert(wl_align_start_cycles == accepted * (SystemCoreClock / 1000U));
    assert(first_move_ms >= accepted && first_move_ms < accepted + WL_ALIGN_DELAY_MS);
    assert(now_ms == accepted + WL_ALIGN_DELAY_MS + 40U);
    assert(wl_wheel_wait_state == WL_WHEEL_DONE);
}

int main(void)
{
    assert(WL_FIRST_EARLY_WINDOW_MS == 2000U && WL_FIRST_RETRY_AFTER_MS == 4000U);
    gate_cases();
    integration(500U, 4000U, 1);
    integration(2000U, 4001U, 1);
    integration(2001U, 2001U, 0); /* New visit must not retain the previous skip. */
    integration(4500U, 4500U, 0);
    puts("PASS: frame validity, 2s/4s boundaries, stale/cached frames, wraparound;");
    puts("PASS: four full WL_dingwei runs, per-visit reset, wheel/positioning timing.");
    return 0;
}
