"""Execute ceshi6 with mocked hardware for all 36 color/ring permutations.

Only the simulated scan literal and function name vary between test cases.
The route and handling calls come directly from the selected task.c.
"""
import argparse
import itertools
from pathlib import Path
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
    raise ValueError("Unsupported encoding")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path,
                        default=Path(__file__).resolve().parents[2] / "stm32")
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = read_source(args.source_root / "BSP/task.c")
    start = source.index("void ceshi6(void)")
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    function = source[start:end]
    permutations = ["".join(p) for p in itertools.permutations("123")]
    definitions, calls = [], []
    for index, (colors, rings) in enumerate(itertools.product(permutations, repeat=2)):
        scan = colors + "+" + rings
        name = "case_" + str(index)
        variant = function.replace("void ceshi6(void)", "static void " + name + "(void)", 1)
        variant, count = re.subn(r'const char simulated_scan\[\] = "[^"]*";',
                                'const char simulated_scan[] = "' + scan + '";', variant)
        assert count == 1
        definitions.append(variant)
        calls.append('    run_case("' + scan + '", ' + name + ');')
    harness = Path(__file__).with_name("ceshi6_tray_order_harness.c").read_text(encoding="utf-8")
    content = harness.replace("/* INSERT_PRODUCTION_FUNCTIONS */", "\n\n".join(definitions))
    content = content.replace("/* INSERT_CASES */", "\n".join(calls))
    with tempfile.TemporaryDirectory(prefix="ceshi6-trays-") as directory:
        work = Path(directory)
        c_file, exe = work / "test.c", work / "test.exe"
        c_file.write_text(content, encoding="utf-8")
        subprocess.run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        str(c_file), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
