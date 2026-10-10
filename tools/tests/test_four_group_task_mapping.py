"""Simulate second-round coarse processing and stacking for all scan permutations."""

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

static uint8_t saoma_data[16];
static int fang2 = -20, fang3 = 17;
static int current_ring, phase, held, trays[4], original[4];
static int rings[4], first_layer[4], second_layer[4];
static int placed, retrieved, stacked;

static void delay_ms(int ms) { assert(ms >= 0); }
static void fuwei(void) {}
static void move(int mode, int pulses)
{
    assert((mode == 1 || mode == 2) &&
           (pulses == 1850 * 4 || pulses == 3700 * 4));
    current_ring += (mode == 2 ? 1 : -1) * pulses / (1850 * 4);
    assert(current_ring >= 1 && current_ring <= 3);
}
static void car(int mode, int rpm, int pulses)
{
    assert(rpm == 500);
    move(mode, pulses);
}
static void vision(char mode)
{
    if (phase == 0) assert(mode == '3' + current_ring);
    else assert(mode == first_layer[current_ring] - '0');
}
static void vision_at(char mode, int center_x, int center_y)
{
    assert(center_x == 175 && center_y == 139);
    vision(mode);
}
static void put_on_coarse_ring(int tray)
{
    assert(phase == 0 && trays[tray] && !rings[current_ring]);
    assert(current_ring == saoma_data[11 + tray] - '0');
    rings[current_ring] = trays[tray]; trays[tray] = 0;
    ++placed;
}
static void fwuliao1(void) { put_on_coarse_ring(3); }
static void fwuliao2(void) { put_on_coarse_ring(2); }
static void fwuliao3(void) { put_on_coarse_ring(1); }
static void take_from_coarse_ring(int tray)
{
    assert(phase == 0 && !held && rings[current_ring] == original[tray]);
    held = rings[current_ring]; rings[current_ring] = 0;
    ++retrieved;
}
static void fnwuliao1(void) { take_from_coarse_ring(1); }
static void fnwuliao2(void) { take_from_coarse_ring(2); }
static void fnwuliao3(void)
{
    take_from_coarse_ring(3);
    trays[3] = held; held = 0;
}
static void zancun_na(int mode, int rpm, int pulses, int tray)
{
    assert(rpm == 500 && (tray == 1 || tray == 2));
    assert(held == original[tray] && !trays[tray]);
    trays[tray] = held; held = 0;
    move(mode, pulses);
}
static void stack_from_tray(int tray)
{
    assert(phase == 1 && trays[tray] == original[tray]);
    assert(current_ring == saoma_data[11 + tray] - '0');
    assert(first_layer[current_ring] && !second_layer[current_ring]);
    second_layer[current_ring] = trays[tray]; trays[tray] = 0;
    ++stacked;
}
static void maduo1(void) { stack_from_tray(1); }
static void maduo2(void) { stack_from_tray(2); }
static void maduo3(void) { stack_from_tray(3); }
static void maduo_z(int angle)
{
    assert(angle == fang2 || angle == fang3);
    stack_from_tray(angle == fang2 ? 2 : 3);
}

/* INSERT_COLOR_LOOKUP */
/* INSERT_COARSE_ACTIONS */
/* INSERT_STACK_ACTIONS */

static void run_case(const char *first_colors, const char *first_rings,
                     const char *second_colors, const char *second_rings)
{
    int i;
    memset(saoma_data, 0, sizeof(saoma_data));
    memcpy(saoma_data, first_colors, 3);
    saoma_data[3] = '+';
    memcpy(saoma_data + 4, first_rings, 3);
    saoma_data[7] = '+';
    memcpy(saoma_data + 8, second_colors, 3);
    saoma_data[11] = '+';
    memcpy(saoma_data + 12, second_rings, 3);
    memset(trays, 0, sizeof(trays));
    memset(rings, 0, sizeof(rings));
    memset(first_layer, 0, sizeof(first_layer));
    memset(second_layer, 0, sizeof(second_layer));
    for (i = 1; i <= 3; ++i) {
        original[i] = second_colors[i - 1] - '0';
        trays[i] = original[i];
        first_layer[first_rings[i - 1] - '0'] = first_colors[i - 1];
    }
    phase = 0; current_ring = 2; held = 0;
    placed = retrieved = stacked = 0;
    second_coarse_actions();
    assert(placed == 3 && retrieved == 3 && !held && current_ring == 2);
    for (i = 1; i <= 3; ++i)
        assert(trays[i] == original[i] && !rings[i]);

    phase = 1;
    second_stack_actions();
    assert(stacked == 3 && current_ring == second_rings[2] - '0');
    for (i = 0; i < 3; ++i) {
        assert(!trays[i + 1]);
        assert(second_layer[second_rings[i] - '0'] == second_colors[i] - '0');
    }
}

int main(void)
{
    const char *cases[] = {"123", "132", "213", "231", "312", "321"};
    const char *colors[] = {"123", "156", "516", "456", "654", "246"};
    int a, b, c, d;
    for (a = 0; a < 6; ++a)
        for (b = 0; b < 6; ++b)
            for (c = 0; c < 6; ++c)
                for (d = 0; d < 6; ++d)
                    run_case(colors[a], cases[b], colors[c], cases[d]);
    puts("PASS: 1296 four-group combinations, including colors 1..6, preserve tray mapping and stack at group-4 rings.");
    return 0;
}
'''


def function(source, name):
    start = source.index(name)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def actions(source, name):
    body = function(source, name)
    marker = "//***********放物料*************"
    return body[body.index(marker):body.rindex("\n}")]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path,
                        default=Path(__file__).resolve().parents[2] / "stm32")
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = (args.source_root / "BSP/task.c").read_bytes().decode("gb18030")
    source = source.replace("\r\r\n", "\n").replace("\r\n", "\n")
    lookup = function(source, "static uint8_t first_round_color_for_ring(uint8_t ring)")
    coarse = actions(source, "void cujiagong2()")
    stack = actions(source, "void zancunqu2()")
    first_actions = actions(source, "void cujiagong()")
    for first_index, second_index in ((6, 14), (5, 13), (4, 12)):
        first_actions = first_actions.replace(
            f"saoma_data[{first_index}]", f"saoma_data[{second_index}]")
    assert re.sub(r"\s+", "", first_actions) == re.sub(r"\s+", "", coarse)
    first_coarse = function(source, "void cujiagong()")
    second_coarse = function(source, "void cujiagong2()")
    lift = "Emm_V5_Pos_Control(5, 1, 500, 200, 8200, 0, 0);"
    assert lift in first_coarse and lift in second_coarse
    assert second_coarse.index(lift) < second_coarse.index("delay_ms(650);")
    assert "const int ring_center_x = 175;" in second_coarse
    assert "const int ring_center_y = 139;" in second_coarse
    assert "vision('4');" not in coarse
    assert "vision('5');" not in coarse
    assert "vision('6');" not in coarse
    code = HARNESS.replace("/* INSERT_COLOR_LOOKUP */", lookup)
    code = code.replace("/* INSERT_COARSE_ACTIONS */",
                        "static void second_coarse_actions(void)\n{\n"
                        "const int ring_center_x = 175, ring_center_y = 139;\n"
                        + coarse + "\n}")
    code = code.replace("/* INSERT_STACK_ACTIONS */",
                        "static void second_stack_actions(void)\n{\n" + stack + "\n}")
    with tempfile.TemporaryDirectory(prefix="four-group-task-") as directory:
        c_file = Path(directory) / "test.c"
        exe = Path(directory) / "test.exe"
        c_file.write_text(code, encoding="utf-8")
        subprocess.run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        str(c_file), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
