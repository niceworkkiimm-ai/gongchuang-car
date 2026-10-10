"""Run the real temporary-zone placement branches with mocked hardware."""

import argparse
import itertools
from pathlib import Path
import subprocess
import tempfile


HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t saoma_data[8];
static int current_ring, trays[4], rings[4], placement[3], count;

static void fuwei(void) {}
static void car(int mode, int speed, int pulses)
{
    assert((mode == 1 || mode == 2) && speed == 500);
    assert(pulses == 1850 * 4 || pulses == 3700 * 4);
    current_ring += (mode == 2 ? 1 : -1) * pulses / (1850 * 4);
    assert(current_ring >= 1 && current_ring <= 3);
}
static void vision_at(char mode, int x, int y)
{
    assert(mode == '3' + current_ring && x == 175 && y == 139);
}
static void place(int tray)
{
    assert(count < 3 && trays[tray] && !rings[current_ring]);
    placement[count++] = current_ring;
    rings[current_ring] = trays[tray];
    trays[tray] = 0;
}
/* Current physical tray choices in wuliao.c: fang3, fang2, fang1. */
static void fwuliao1(void) { place(3); }
static void fwuliao2(void) { place(2); }
static void fwuliao3(void) { place(1); }

/* INSERT_ACTIONS */

int main(void)
{
    const char *cases[] = {"123", "132", "213", "231", "312", "321"};
    int i, j;
    for (i = 0; i < 6; ++i) {
        memset(saoma_data, 0, sizeof(saoma_data));
        memcpy(saoma_data, cases[i], 3);
        memset(trays, 0, sizeof(trays));
        memset(rings, 0, sizeof(rings));
        current_ring = 2; count = 0;
        for (j = 1; j <= 3; ++j) trays[j] = cases[i][j - 1] - '0';
        place_at_temporary_zone();
        assert(count == 3 && current_ring == cases[i][0] - '0');
        for (j = 1; j <= 3; ++j) {
            assert(trays[j] == 0);
            assert(rings[j] == j);
            assert(placement[j - 1] == cases[i][3 - j] - '0');
        }
    }
    puts("PASS: all six temporary-zone placements use the matching tray/ring and leave materials there.");
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path,
                        default=Path(__file__).resolve().parents[2] / "stm32")
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    wuliao = (args.source_root / "hardware/wuliao.c").read_bytes()
    for name, tray_angle in (("fwuliao1", "fang3"),
                             ("fwuliao2", "fang2"),
                             ("fwuliao3", "fang1")):
        function_start = wuliao.index(("void " + name + "(void)").encode())
        function_end = wuliao.index(b"\n}", function_start)
        assert ("zhou(" + tray_angle + ")").encode() in wuliao[function_start:function_end]
    source = (args.source_root / "BSP/task.c").read_bytes().decode("gb18030")
    source = source.replace("\r\r\n", "\n").replace("\r\n", "\n")
    start = source.index("void zancunqu()")
    end = source.index("//**************************************", start)
    function = source[start:end]
    marker = "//***********放物料*************"
    action = function[function.index(marker):function.rindex("\n}")]
    assert "fnwuliao" not in action and "zancun_na" not in action
    assert action.count("fwuliao1();") == 3
    assert action.count("fwuliao2();") == 6
    assert action.count("fwuliao3();") == 6
    definition = ("static void place_at_temporary_zone(void)\n"
                  "{\n    const int ring_center_x = 175;\n"
                  "    const int ring_center_y = 139;\n" + action + "\n}")
    code = HARNESS.replace("/* INSERT_ACTIONS */", definition)
    with tempfile.TemporaryDirectory(prefix="zancunqu-place-") as directory:
        c_file = Path(directory) / "test.c"
        exe = Path(directory) / "test.exe"
        c_file.write_text(code, encoding="utf-8")
        subprocess.run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        str(c_file), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
