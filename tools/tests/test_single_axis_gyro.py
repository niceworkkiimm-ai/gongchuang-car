"""Compile the production UART3 parser/command sender with host UART stubs."""

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define GYRO_YAW_DIRECTION /* DIRECTION */
#define USART_SR_RXNE 0x20U
#define USART_SR_ORE 0x08U
#define USART_SR_FE 0x02U
#define USART_SR_NE 0x04U
#define USART_SR_PE 0x01U
#define USART_FLAG_TC 0x40U
#define RESET 0
static struct { uint16_t SR, DR; } uart;
#define USART3 (&uart)
volatile float global_angle, angular_velocity_z;
volatile uint8_t new_data_received;
volatile uint32_t gyro_yaw_frame_count;
/* The separate logger has its own production-module tests. */
static void GyroLog_OnYaw(void) {}
static uint8_t sent[64];
static unsigned sent_count, waits[8], wait_count;
static void Usart3_SendByte(uint8_t byte) { sent[sent_count++] = byte; }
static int USART_GetFlagStatus(void *port, unsigned flag)
{ assert(port == USART3 && flag == USART_FLAG_TC); return 1; }
static void delay_ms(unsigned ms) { waits[wait_count++] = ms; }
void ParseAndPrintData(uint8_t *data, uint16_t length);
uint8_t CalculateChecksum(uint8_t *data, uint16_t length, uint8_t type);

/* CONFIG */
/* PARSER */
static void setup_gyro(void) { /* SETUP */ }

static void reset_rx(void)
{
    gyro_rx_count = 0;
    gyro_yaw_frame_count = 0;
    new_data_received = 0;
    global_angle = angular_velocity_z = 0.0f;
}
static void feed(const uint8_t *bytes, unsigned count)
{
    unsigned i;
    for (i = 0; i < count; ++i) {
        uart.SR = USART_SR_RXNE; uart.DR = bytes[i];
        USART3_IRQHandler();
    }
}
static void frame(uint8_t *out, uint8_t type, int raw)
{
    out[0] = 0x5A; out[1] = type;
    out[2] = (uint8_t)raw; out[3] = (uint8_t)((uint16_t)raw >> 8);
    out[4] = (uint8_t)(out[0] + out[1] + out[2] + out[3]);
}
static float expected_yaw(int raw)
{
    float value = GYRO_YAW_DIRECTION * raw * (180.0f / 32768.0f);
    return value >= 180.0f ? value - 360.0f : value;
}
int main(void)
{
    uint8_t yaw[5], rate[5], bad[5], stream[16];
    int raw, i, drop;
    const uint8_t expected_commands[] = {
        0x55,0xAA,0x13,0x8E,0x5F, 0x55,0xAA,0x02,0x0B,0,
        0x55,0xAA,0x15,0,0,       0x55,0xAA,0,0,0
    };
    setup_gyro();
    assert(sent_count == sizeof(expected_commands));
    assert(memcmp(sent, expected_commands, sizeof(expected_commands)) == 0);
    assert(wait_count == 5 && waits[0] == 600);
    for (i = 1; i < 5; ++i) assert(waits[i] == 100);

    /* Exhaust the signed 16-bit yaw range, including fractional resolution. */
    reset_rx();
    for (raw = -32768; raw <= 32767; ++raw) {
        frame(yaw, 0xBB, raw); feed(yaw, 5);
        assert(fabsf(global_angle - expected_yaw(raw)) < 0.0001f);
        assert(global_angle >= -180.0f && global_angle < 180.0f);
    }
    assert(gyro_yaw_frame_count == 65536U);
    assert(new_data_received == 1);

    /* Rate frames update only the Z rate and do not count as yaw frames. */
    reset_rx(); frame(yaw, 0xBB, 1); feed(yaw, 5);
    assert(global_angle != 0.0f);
    for (raw = -32768; raw <= 32767; ++raw) {
        frame(rate, 0xAA, raw); feed(rate, 5);
        assert(fabsf(angular_velocity_z - GYRO_YAW_DIRECTION * raw *
                     (2000.0f / 32768.0f)) < 0.0001f);
        assert(global_angle == expected_yaw(1));
    }
    assert(gyro_yaw_frame_count == 1);

    frame(yaw, 0xBB, 16384);
    for (i = 0; i < 256; ++i) {
        if (i == yaw[4]) continue;
        reset_rx(); memcpy(bad, yaw, 5); bad[4] = (uint8_t)i;
        feed(bad, 5);
        assert(!gyro_yaw_frame_count && !new_data_received);
        feed(yaw, 5);
        assert(gyro_yaw_frame_count == 1 && global_angle == expected_yaw(16384));
    }

    /* Lost or inserted bytes must not hide the next intact packet. */
    for (drop = 0; drop < 5; ++drop) {
        unsigned n = 0;
        reset_rx();
        for (i = 0; i < 5; ++i) if (i != drop) stream[n++] = yaw[i];
        memcpy(stream + n, yaw, 5); feed(stream, n + 5);
        assert(gyro_yaw_frame_count == 1 && global_angle == expected_yaw(16384));
    }
    for (i = 0; i <= 5; ++i) {
        reset_rx();
        memcpy(stream, yaw, (unsigned)i); stream[i] = 0x00;
        memcpy(stream + i + 1, yaw + i, (unsigned)(5 - i));
        feed(stream, 6); feed(yaw, 5);
        assert(global_angle == expected_yaw(16384));
    }
    reset_rx();
    frame(bad, 0xCC, 0); feed(bad, 5); /* Undocumented status frame. */
    assert(!new_data_received && !gyro_yaw_frame_count);
    frame(bad, 0xBB, 0x5A5A); feed(bad, 5); /* Header bytes inside data. */
    assert(gyro_yaw_frame_count == 1 && global_angle == expected_yaw(0x5A5A));

    /* USART receive errors discard the partial frame, then recover. */
    reset_rx(); feed(yaw, 2);
    uart.SR = USART_SR_RXNE | USART_SR_ORE; uart.DR = 0xFF;
    USART3_IRQHandler();
    assert(gyro_rx_count == 0 && !new_data_received);
    feed(yaw, 5); assert(gyro_yaw_frame_count == 1);
    reset_rx();
    ParseAndPrintData(yaw, 4); assert(!gyro_yaw_frame_count);
    puts("PASS: startup commands, full yaw/rate range, checksum, resync, UART errors");
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[2] /
                        "stm32/hardware/uart_3.c")
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = args.source.read_bytes().decode("gb18030").replace("\r\n", "\n")
    header = args.source.with_suffix(".h").read_bytes().decode("gb18030")
    direction = re.search(r"#define\s+GYRO_YAW_DIRECTION\s+([^\r\n]+)", header).group(1)
    config = source[source.index("static uint8_t unlock_register"):source.index("void Usart3_Init")]
    setup = source[source.index("    delay_ms(600);"):source.index("void Usart3_SendByte")]
    setup = setup[:setup.rfind("}")]
    production = source[source.index("void Usart3_SendArray"):]
    text = (HARNESS.replace("/* CONFIG */", config)
            .replace("/* PARSER */", production).replace("/* SETUP */", setup))
    with tempfile.TemporaryDirectory(prefix="gyro-uart3-") as temp:
        root = Path(temp)
        for label, sign in [("configured", direction), ("reverse", "-1.0f")]:
            test = root / "gyro_test.c"
            test.write_text(text.replace("/* DIRECTION */", sign), encoding="utf-8")
            exe = root / "gyro_test.exe"
            subprocess.run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                            str(test), "-o", str(exe), "-lm"], check=True)
            print(label, flush=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
