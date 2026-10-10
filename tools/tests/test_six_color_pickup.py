"""Exercise the production second/third pickup wait for camera modes 1..6."""

import argparse
from pathlib import Path
import subprocess
import tempfile


HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static volatile uint32_t camera_debug_frames;
static volatile uint8_t maixcam_mode, maixcam_found;
static uint8_t uart4_RxDataopenmv[10], sent_mode;
static int polls;
#define UART4 ((void *)1)

static void Usart_SendByte4(void *uart, uint8_t command)
{
    assert(uart == UART4);
    sent_mode = command >= '7' && command <= '9' ? command - '7' + 1 : command;
    maixcam_mode = sent_mode;
    maixcam_found = 0;
    uart4_RxDataopenmv[0] = 0;
    polls = 0;
}

static void fuwei(void)
{
    ++polls;
    if (polls == 1) return; /* No new frame: cached state must not pass. */
    ++camera_debug_frames;
    maixcam_found = polls >= 3;
    uart4_RxDataopenmv[0] = maixcam_found ?
        (sent_mode <= 3 ? (uint8_t)(0xA3 + sent_mode) : 0xA3) : 0;
}

/* INSERT_PRODUCTION_HELPER */

int main(void)
{
    uint8_t color;
    for (color = '1'; color <= '6'; ++color) {
        WaitMaterialColor(color);
        assert(sent_mode == color - '0');
        assert(polls == 3);
    }
    puts("PASS: both later pickups wait for fresh camera detections in color modes 1..6.");
    return 0;
}
'''


def extract(source, name):
    start = source.index(name)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path,
                        default=Path(__file__).resolve().parents[2] / "stm32")
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    task = (args.source_root / "BSP/task.c").read_bytes().decode("gb18030")
    camera = (args.source_root / "BSP/openmv.c").read_bytes().decode("gb18030")
    helper = extract(task, "static void WaitMaterialColor(uint8_t color)")
    assert task.count("WaitMaterialColor(saoma_data[1]);") == 1
    assert task.count("WaitMaterialColor(saoma_data[2]);") == 1
    assert task.count("WaitMaterialColor(saoma_data[9]);") == 1
    assert task.count("WaitMaterialColor(saoma_data[10]);") == 1
    first_pickup = extract(camera, "void WL_dingwei(char WL)")
    assert "Usart_SendByte4(UART4, (uint8_t)(WL - '0'));" in first_pickup
    source = HARNESS.replace("/* INSERT_PRODUCTION_HELPER */", helper)
    with tempfile.TemporaryDirectory(prefix="six-color-pickup-") as directory:
        c_file = Path(directory) / "test.c"
        exe = Path(directory) / "test.exe"
        c_file.write_text(source, encoding="utf-8")
        subprocess.run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        str(c_file), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
