"""Run the production locator against a simulated clock and camera (host GCC).

python tools/tests/test_wl_first_target_gate.py --source-root <firmware> --cc <gcc>
No MCU, camera or motor hardware is exercised by this test.
"""
import argparse
import pathlib
import re
import subprocess
import tempfile


def read_source(path):
    data = path.read_bytes()
    for encoding in ("utf-8-sig", "gb18030"):
        try:
            return data.decode(encoding).replace("\r\r\n", "\n").replace("\r\n", "\n")
        except UnicodeDecodeError:
            pass
    raise ValueError("Unsupported source encoding: " + str(path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[2] / "stm32")
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = read_source(args.source_root / "BSP/openmv.c")
    header = read_source(args.source_root / "BSP/openmv.h")
    definitions = []
    for name in ("WL_WHEEL_IDLE", "WL_WHEEL_WAIT", "WL_WHEEL_DONE",
                 "WL_ALIGN_DELAY_MS", "WL_FIRST_EARLY_WINDOW_MS",
                 "WL_FIRST_RETRY_AFTER_MS", "VISION_FRAME_STALE_MS"):
        definitions.append(re.search(r"^#define " + name + r"\s+[^\n]+",
                                     source + "\n" + header, re.M).group(0))
    locator = source[source.index("volatile uint8_t wl_wheel_wait_state"):]
    # Substitute the two memory-mapped registers; execute all production logic.
    locator = re.sub(r"^#define WL_DWT_CTRL[^\n]+", "#define WL_DWT_CTRL fake_ctrl", locator, flags=re.M)
    locator = re.sub(r"^#define WL_DWT_CYCCNT[^\n]+", "#define WL_DWT_CYCCNT fake_cycles", locator, flags=re.M)
    with tempfile.TemporaryDirectory(prefix="wl-first-target-") as temp:
        work = pathlib.Path(temp)
        (work / "locator_under_test.c").write_text("\n".join(definitions) + "\n" + locator,
                                                   encoding="utf-8")
        harness = pathlib.Path(__file__).with_name("wl_first_target_gate_harness.c")
        exe = work / "gate_test.exe"
        subprocess.run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        "-I", str(work), str(harness), "-lm", "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
