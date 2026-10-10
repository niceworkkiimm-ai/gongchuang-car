"""Exercise the production UART5 scan parser with a 15-character task code."""

import argparse
from pathlib import Path
import subprocess
import tempfile


HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t scan_window[15], scan_count;
static uint8_t saoma_data[1000];
static uint8_t saoma_ready;
static char hmi_scan[16];
static int hmi_count;

static void HMI_QueueScan(const uint8_t *scan)
{
    memcpy(hmi_scan, scan, 15);
    hmi_scan[15] = '\0';
    ++hmi_count;
}

/* INSERT_PRODUCTION_PARSER */

static void reset(void)
{
    memset(scan_window, 0, sizeof(scan_window));
    memset(saoma_data, 0, sizeof(saoma_data));
    memset(hmi_scan, 0, sizeof(hmi_scan));
    scan_count = saoma_ready = 0;
    hmi_count = 0;
}

static void feed(const char *input)
{
    while (*input) Scanner_ReceiveByte((uint8_t)*input++);
}

int main(void)
{
    reset();
    feed("123+321");
    assert(!saoma_ready && !hmi_count);
    feed("+132+213");
    assert(saoma_ready && hmi_count == 1);
    assert(strcmp((char *)saoma_data, "123+321+132+213") == 0);
    assert(strcmp(hmi_scan, (char *)saoma_data) == 0);
    feed("321+123+132+213");
    assert(strcmp((char *)saoma_data, "123+321+132+213") == 0);
    assert(strcmp(hmi_scan, "321+123+132+213") == 0);

    reset();
    feed("123+321+132+212");
    assert(!saoma_ready && hmi_count == 1);
    reset();
    feed("156+123+516+231");
    assert(saoma_ready && hmi_count == 1);
    assert(strcmp((char *)saoma_data, "156+123+516+231") == 0);
    reset();
    feed("116+123+516+231");
    assert(!saoma_ready && hmi_count == 1);
    reset();
    feed("167+123+516+231");
    assert(!saoma_ready && !hmi_count);
    reset();
    feed("156+124+516+231");
    assert(!saoma_ready && !hmi_count);
    reset();
    feed("prefix123+321+132+213\r\n");
    assert(saoma_ready && strcmp((char *)saoma_data, "123+321+132+213") == 0);
    reset();
    feed("123+321+132-213");
    assert(!saoma_ready && !hmi_count);

    puts("PASS: full four-group scan locks only after 15 bytes; display and invalid-input handling verified.");
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path,
                        default=Path(__file__).resolve().parents[2] / "stm32")
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = (args.source_root / "hardware/uart_5.c").read_bytes()
    assert b"#define HMI_SCAN_LENGTH 15U" in source
    start = source.index(b"static void Scanner_ReceiveByte(uint8_t data)")
    end = source.index(b"\n}", start) + 2
    production = source[start:end].decode("ascii").replace("\r\r\n", "\n").replace("\r\n", "\n")
    code = HARNESS.replace("/* INSERT_PRODUCTION_PARSER */", production)
    with tempfile.TemporaryDirectory(prefix="four-group-scan-") as directory:
        c_file = Path(directory) / "test.c"
        exe = Path(directory) / "test.exe"
        c_file.write_text(code, encoding="utf-8")
        subprocess.run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        str(c_file), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
