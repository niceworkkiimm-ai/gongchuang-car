#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t saoma_data[1000], saoma_ready;
static int UART4, guiwei = 162;
static int current_ring, held, trays[4], rings[4], original[4];
static int place_count, grab_count, camera_count, preparation_count;
static int placement[3], retrieval[3];
static uint32_t irq_state;

static uint32_t __get_PRIMASK(void) { return irq_state; }
static void __disable_irq(void) { irq_state = 1; }
static void __set_PRIMASK(uint32_t value) { irq_state = value; }
static void delay_ms(int ms) { assert(ms >= 0); }
static void fuwei(void) {}
static void zhou(int angle) { assert(angle == guiwei); }
static void Usart_SendByte4(int port, uint8_t mode)
{ assert(port == UART4 && mode == 7); ++camera_count; }
static void Emm_V5_Pos_Control(int addr, int dir, int rpm, int acc,
                               int pulses, int absolute, int sync)
{
    assert(addr == 5 && dir == 1 && rpm == 500 && acc == 200);
    assert(pulses == 8200 && !absolute && !sync);
    ++preparation_count;
}
static void move_ring(int direction, int pulses)
{
    assert(direction == 1 || direction == 2);
    assert(pulses == 1850 * 4 || pulses == 3700 * 4);
    current_ring += (direction == 2 ? 1 : -1) * pulses / (1850 * 4);
    assert(current_ring >= 1 && current_ring <= 3);
}
static void car(int direction, int speed, int pulses)
{
    assert((direction == 1 || direction == 2) && speed == 500);
    move_ring(direction, pulses);
}
static void vision_at(char mode, int center_x, int center_y)
{
    assert(mode - '3' == current_ring);
    assert(center_x == 175 && center_y == 139);
}
static void put_on_ring(int tray)
{
    assert(place_count < 3 && !rings[current_ring] && trays[tray]);
    placement[place_count++] = current_ring;
    rings[current_ring] = trays[tray]; trays[tray] = 0;
}
/* Current physical tray selections in wuliao.c are fang3, fang2, fang1. */
static void fwuliao1(void) { put_on_ring(3); }
static void fwuliao2(void) { put_on_ring(2); }
static void fwuliao3(void) { put_on_ring(1); }
static void pick_for_tray(int tray)
{
    assert(place_count == 3 && !held && grab_count < 3);
    assert(rings[current_ring] == original[tray]);
    held = rings[current_ring]; rings[current_ring] = 0;
    retrieval[grab_count++] = current_ring;
}
static void fnwuliao1(void) { pick_for_tray(1); }
static void fnwuliao2(void) { pick_for_tray(2); }
static void fnwuliao3(void)
{
    pick_for_tray(3);
    assert(!trays[3]); trays[3] = held; held = 0;
}
static void zancun_na(int direction, int speed, int pulses, int tray)
{
    assert(tray == 1 || tray == 2);
    assert(speed == 500 && held == original[tray] && !trays[tray]);
    trays[tray] = held; held = 0;
    move_ring(direction, pulses);
}

/* INSERT_PRODUCTION_FUNCTIONS */

static void run_case(const char *scan, void (*function)(void))
{
    int i;
    memset(saoma_data, 0, sizeof(saoma_data));
    memset(rings, 0, sizeof(rings));
    saoma_ready = 0; irq_state = 0; current_ring = 2; held = 0;
    place_count = grab_count = camera_count = preparation_count = 0;
    for (i = 1; i <= 3; ++i) trays[i] = original[i] = scan[i - 1] - '0';
    function();
    assert(saoma_ready && strcmp((char *)saoma_data, scan) == 0 && !irq_state);
    assert(camera_count == 1 && preparation_count == 1);
    assert(place_count == 3 && grab_count == 3 && !held);
    for (i = 0; i < 3; ++i) {
        assert(placement[i] == scan[6 - i] - '0');
        assert(retrieval[i] == scan[4 + i] - '0');
        assert(trays[i + 1] == original[i + 1] && !rings[i + 1]);
    }
    assert(current_ring == 2);
}

int main(void)
{
    /* INSERT_CASES */
    puts("PASS: all 36 color/ring permutations restore trays 1, 2, 3;");
    puts("PASS: placement order, pickup order, no -90 turn and return to ring 2.");
    return 0;
}
