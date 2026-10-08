"""PC 回归：稳定门控、真实检测缓存、主循环显示及串口协议。"""

import contextlib
import importlib
import io
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from serial_link import SerialLink
from target_stability import StableTargetLock


def target(x=180, y=105, color=1):
    return {"cx": x, "cy": y, "color_id": color,
            "area": 2000, "r": 25, "name": "test"}


class StabilityTests(unittest.TestCase):
    def setUp(self):
        self.lock = StableTargetLock(5, 3)

    def test_first_four_rejected_fifth_uses_current_position(self):
        for frame, x in enumerate([180, 181, 182, 180], 1):
            self.assertIsNone(self.lock.update(target(x), frame))
        current = target(183)
        result = self.lock.update(current, 5)
        self.assertEqual(result, current)
        self.assertIsNot(result, current)

    def test_window_range_not_only_adjacent_difference(self):
        for frame in range(1, 31):
            self.assertIsNone(self.lock.update(target(frame), frame))

    def test_stationary_jitter_continues_output(self):
        for frame in range(1, 31):
            result = self.lock.update(target(180 + frame % 4, 105 + frame % 3), frame)
            self.assertEqual(result is not None, frame >= 5)

    def test_x_or_y_motion_breaks_lock_and_requires_five_new_frames(self):
        for axis in ("cx", "cy"):
            with self.subTest(axis=axis):
                self.lock.reset()
                for frame in range(1, 6):
                    self.lock.update(target(), frame)
                moved = target()
                moved[axis] += 4
                for frame in range(6, 10):
                    self.assertIsNone(self.lock.update(moved, frame))
                self.assertEqual(self.lock.update(moved, 10), moved)

    def test_lost_or_invalid_resets(self):
        for missing in (None, dict(target(), valid=False)):
            with self.subTest(missing=missing):
                self.lock.reset()
                for frame in range(1, 6):
                    self.lock.update(target(), frame)
                self.assertIsNone(self.lock.update(missing, 6))
                for frame in range(7, 11):
                    self.assertIsNone(self.lock.update(target(), frame))
                self.assertIsNotNone(self.lock.update(target(), 11))

    def test_skipped_frame_requires_new_window(self):
        for frame in range(1, 5):
            self.lock.update(target(), frame)
        for frame in range(6, 10):
            self.assertIsNone(self.lock.update(target(), frame))
        self.assertIsNotNone(self.lock.update(target(), 10))

    def test_same_frame_cannot_be_counted_repeatedly(self):
        for _ in range(20):
            self.assertIsNone(self.lock.update(target(), 1))

    def test_color_change_requires_new_window(self):
        for frame in range(1, 6):
            self.lock.update(target(color=1), frame)
        for frame in range(6, 10):
            self.assertIsNone(self.lock.update(target(color=6), frame))
        self.assertIsNotNone(self.lock.update(target(color=6), 10))

    def test_explicit_reset(self):
        for frame in range(1, 6):
            self.lock.update(target(), frame)
        self.lock.reset()
        self.assertIsNone(self.lock.update(target(), 6))

    def test_three_arrivals_at_same_location(self):
        frame = 0
        for _ in range(3):
            for x in (100, 120, 140, 160):
                frame += 1
                self.assertIsNone(self.lock.update(target(x), frame))
            for count in range(1, 11):
                frame += 1
                result = self.lock.update(target(180), frame)
                self.assertEqual(result is not None, count >= 5)
            frame += 1
            self.assertIsNone(self.lock.update(None, frame))

    def test_configuration_validation(self):
        for frames, tolerance in ((0, 3), (-1, 3), (5.0, 3), (True, 3), (5, -1), (5, 1.5)):
            with self.subTest(frames=frames, tolerance=tolerance):
                with self.assertRaises(ValueError):
                    StableTargetLock(frames, tolerance)

    def test_configurable_one_frame_and_exact_tolerance(self):
        self.assertEqual(StableTargetLock(1, 0).update(target(), 1), target())
        lock = StableTargetLock(5, 0)
        for frame in range(1, 5):
            self.assertIsNone(lock.update(target(), frame))
        self.assertIsNone(lock.update(target(181), 5))


def make_link():
    link = SerialLink.__new__(SerialLink)
    link._frame_interval = 10
    link._frame_count = 0
    link.tx_count = 0
    packets = []
    link._serial = types.SimpleNamespace(write=packets.append)
    return link, packets


class SerialTests(unittest.TestCase):
    def test_packet_format_signed_offsets_and_checksum(self):
        self.assertEqual(SerialLink.encode_result(True, 20, -15).hex(), "aa010014fff11b55")
        self.assertEqual(SerialLink.encode_result(False).hex(), "aa00000000000055")

    def test_stable_gate_keeps_ten_frame_cadence(self):
        lock = StableTargetLock()
        link, packets = make_link()
        # 第 7 帧开始静止：第 10 帧仍未满 5 帧，第 20 帧有效，第 30 帧丢失。
        for frame in range(1, 31):
            raw = target(frame * 10) if frame < 7 else target()
            if frame == 30:
                raw = None
            current = lock.update(raw, frame)
            sent = link.send_result(current, 320, 240)
            self.assertEqual(sent, frame % 10 == 0)
        self.assertEqual([p[1] for p in packets], [0, 1, 0])
        self.assertEqual(packets[1].hex(), "aa010014fff11b55")
        self.assertEqual(packets[-1].hex(), "aa00000000000055")

    def test_single_byte_commands_unchanged(self):
        link, _ = make_link()
        link.config = {"accept_ascii": True}
        for mode in range(8):
            for message in (bytes([mode]), str(mode).encode("ascii")):
                link._serial.read = lambda: message
                self.assertEqual(link.poll_mode(), mode)


class ProjectTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # 只模拟硬件边界；运行真实 color_detector、main 和 SerialLink。
        fake_maix = types.ModuleType("maix")
        for name in ("app", "camera", "display", "image", "time", "nn"):
            setattr(fake_maix, name, types.SimpleNamespace())
        with patch.dict(sys.modules, {"maix": fake_maix}):
            cls.cd = importlib.import_module("color_detector")
            cls.main = importlib.import_module("main")

    def setUp(self):
        self.cd.reset_cache()

    def test_tracker_miss_never_returns_cache_and_next_frame_full_scans(self):
        first, second = target(), target(200)
        with patch.object(self.cd, "_detect_one_color", side_effect=[first, None, None, second]) as detect:
            self.assertEqual(self.cd.detect_colors(object(), 1), [first])
            self.assertEqual(self.cd.detect_colors(object(), 1), [])
            self.assertEqual(self.cd._color_cache, [])
            self.assertEqual(self.cd.detect_colors(object(), 1), [second])
        self.assertNotIn("roi", detect.call_args_list[-1].kwargs)
        self.assertEqual(len(detect.call_args_list[-1].args), 2)

    def test_full_scan_miss_clears_cache(self):
        self.cd._color_cache = [target()]
        self.cd._frame_cnt = self.cd.FULL_SCAN_EVERY - 1
        with patch.object(self.cd, "_detect_one_color", return_value=None):
            self.assertEqual(self.cd.detect_colors(object(), 1), [])
        self.assertEqual(self.cd._color_cache, [])

    def test_empty_cache_skip_does_not_count_as_observation(self):
        self.cd._frame_cnt = 0
        lock = StableTargetLock()
        with patch.object(self.cd, "_detect_one_color", return_value=None) as detect:
            for frame in range(1, self.cd.NO_CACHE_FULL_SCAN_EVERY):
                colors = self.cd.detect_colors(object(), 1)
                self.assertEqual(colors, [])
                self.assertIsNone(lock.update(None, frame))
        detect.assert_not_called()

    def test_current_centers_from_real_lab_blob_geometry(self):
        blob = types.SimpleNamespace(
            x=lambda:110, y=lambda:70, w=lambda:61, h=lambda:41, area=lambda:2000,
        )
        for cid in (1, 2, 4, 5):
            frame = types.SimpleNamespace(find_blobs=lambda *a, **k:[blob])
            self.cd.reset_cache()
            lock = StableTargetLock()
            for number in range(1, 6):
                result = self.cd.detect_colors(frame, cid)[0]
                self.assertEqual((result["cx"], result["cy"]), (140, 90))
                self.assertEqual(lock.update(result, number) is not None, number == 5)

    def test_hsv_real_images_stable_loss_and_return(self):
        for cid in (3, 6):
            with self.subTest(cid=cid):
                self.cd.reset_cache()
                lock = StableTargetLock()
                hsv = np.zeros((240, 320, 3), dtype=np.uint8)
                lo, hi = self.cd._HSV_BOUNDS[cid]
                hsv[80:121, 120:161] = ((lo.astype(int) + hi.astype(int)) // 2)
                with patch.object(self.cd, "_to_hsv", return_value=hsv) as convert:
                    for number in range(1, 6):
                        result = self.cd.detect_colors(object(), cid)[0]
                        self.assertEqual((result["cx"], result["cy"]), (140, 100))
                        self.assertEqual(lock.update(result, number) is not None, number == 5)
                    self.assertEqual(convert.call_count, 5)
                    hsv[:] = 0
                    self.assertEqual(self.cd.detect_colors(object(), cid), [])
                    self.assertIsNone(lock.update(None, 6))
                    hsv[80:121, 120:161] = ((lo.astype(int) + hi.astype(int)) // 2)
                    for number in range(7, 12):
                        result = self.cd.detect_colors(object(), cid)[0]
                        self.assertEqual(lock.update(result, number) is not None, number == 11)

    def run_main(self, entries, detections=None, labels=None, expected_numbers=None,
                 model_load_error=False):
        main = self.main
        frames, reports = [], []
        link, packets = make_link()
        original_send = link.send_result

        def send(result, width, height):
            reports.append(result)
            return original_send(result, width, height)

        link.send_result = send
        link.poll_mode = lambda: entries[len(frames)][0]

        class Frame:
            def __init__(self):
                self.text = []
                self.crosses = []

            def resize(self, *args):
                return self

            def draw_string(self, x, y, text, color):
                self.text.append(text)

            def draw_cross(self, x, y, color, **kwargs):
                self.crosses.append((x, y, color))

        def detect(*args, **kwargs):
            if detections is None:
                return [types.SimpleNamespace(class_id=0, score=.9, x=70, y=70, w=20, h=40)]
            result = detections[len(frames)]
            if isinstance(result, Exception):
                raise result
            return result

        model = types.SimpleNamespace(
            labels=["one"] if labels is None else labels,
            input_width=lambda:224, input_height=lambda:224, detect=detect,
        )

        def load_model(**kwargs):
            if model_load_error:
                raise RuntimeError("simulated model load failure")
            return model
        resources = {
            "camera": types.SimpleNamespace(Camera=lambda *a:types.SimpleNamespace(read=Frame, close=lambda:None)),
            "display": types.SimpleNamespace(Display=lambda:types.SimpleNamespace(show=frames.append, close=lambda:None)),
            "app": types.SimpleNamespace(need_exit=lambda:len(frames) >= len(entries)),
            "image": types.SimpleNamespace(COLOR_GREEN=1, COLOR_WHITE=2, image2cv=lambda *a:None),
            "time": types.SimpleNamespace(fps=lambda:30),
            "nn": types.SimpleNamespace(YOLOv5=load_model),
            "create_serial_link": lambda cfg:link,
            "detect_colors": lambda img, mode:([entries[len(frames)][1]] if entries[len(frames)][1] is not None else []),
            "reset_cache": lambda:None,
            "reset_ring_state": lambda:None,
            "detect_all": lambda img:[{"inner":True}],
        }
        with contextlib.ExitStack() as stack:
            for key, value in resources.items():
                stack.enter_context(patch.object(main, key, value))
            stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
            main._run(types.SimpleNamespace(set_mode=lambda mode:None))
        for index, ((mode, _), frame, report) in enumerate(zip(entries, frames, reports)):
            self.assertEqual(frame.text[0], "MODE:%d FPS:30" % mode)
            if report is None:
                self.assertEqual(frame.text[1], "dx:-- dy:--")
                self.assertEqual(frame.crosses, [])
            else:
                expected = "dx:%+d dy:%+d" % (report["cx"]-160, report["cy"]-120)
                self.assertEqual(frame.text[1], expected)
                self.assertEqual(frame.crosses, [(report["cx"], report["cy"], 2)] if 1 <= mode <= 6 else [])
            if mode == 7:
                expected_number = "1" if expected_numbers is None else expected_numbers[index]
                self.assertEqual(frame.text[2], "NUM:" + expected_number)
                self.assertEqual(len(frame.text), 3)
            else:
                self.assertEqual(len(frame.text), 2)
        return reports, packets

    def test_main_color_gate_applies_to_all_six_modes(self):
        for cid in range(1, 7):
            with self.subTest(cid=cid):
                entries = [(cid, target(color=cid))]*10
                reports, packets = self.run_main(entries)
                self.assertEqual([r is not None for r in reports], [False]*4+[True]*6)
                self.assertEqual(packets, [SerialLink.encode_result(True, 20, -15)])

    def test_main_moving_then_stable_then_lost(self):
        entries = [(1, target(x*10)) for x in range(1, 11)]
        entries += [(1, target())]*10+[(1, None)]*10
        reports, packets = self.run_main(entries)
        self.assertEqual([r is not None for r in reports], [False]*14+[True]*6+[False]*10)
        self.assertEqual([p[1] for p in packets], [0, 1, 0])

    def test_main_switching_modes_requires_new_lock_including_idle(self):
        entries = [(1, target())]*5+[(2, target(color=2))]*5
        entries += [(0, None)]+[(2, target(color=2))]*5
        reports, _ = self.run_main(entries)
        self.assertEqual([i for i, r in enumerate(reports) if r is not None], [4, 9, 15])

    def test_main_mode7_does_not_require_five_frame_color_lock(self):
        reports, packets = self.run_main([(7, None)]*50)
        self.assertTrue(all(r is None for r in reports[:46]))
        self.assertTrue(all(r is not None for r in reports[46:]))
        self.assertEqual([p[1] for p in packets], [0, 0, 0, 0, 1])

    def test_mode7_displays_all_digit_labels_including_zero(self):
        labels = ["zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"]
        detections = [[types.SimpleNamespace(class_id=i, score=.9, x=70, y=70, w=30, h=40)]
                      for i in range(10)]
        reports, packets = self.run_main(
            [(7, None)]*10, detections=detections, labels=labels,
            expected_numbers=[str(i) for i in range(10)],
        )
        # 数字显示不要求圆检测已确认，也不改变 found 的触发条件。
        self.assertTrue(all(r is None for r in reports))
        self.assertEqual(packets, [SerialLink.encode_result(False)])

    def test_mode7_display_matches_selected_target_with_multiple_digits(self):
        detections = [
            types.SimpleNamespace(class_id=0, score=.8, x=30, y=70, w=30, h=40),
            types.SimpleNamespace(class_id=1, score=.9, x=120, y=70, w=30, h=40),
            types.SimpleNamespace(class_id=2, score=.99, x=170, y=70, w=30, h=40),
        ]
        reports, packets = self.run_main(
            [(7, None)]*50, detections=[detections]*50,
            labels=["one", "two", "background"], expected_numbers=["2"]*50,
        )
        self.assertEqual((reports[-1]["cx"], reports[-1]["cy"]), (192, 96))
        self.assertEqual(packets[-1], SerialLink.encode_result(True, 32, -24))

    def test_mode7_corrected_label_is_displayed(self):
        narrow_two = types.SimpleNamespace(class_id=0, score=.9, x=70, y=70, w=10, h=50)
        self.run_main([(7, None)], detections=[[narrow_two]], labels=["two"], expected_numbers=["1"])

    def test_mode7_uses_corrected_score_and_tie_break_like_position(self):
        narrow_two = types.SimpleNamespace(class_id=0, score=.9, x=30, y=70, w=10, h=50)
        three = types.SimpleNamespace(class_id=1, score=.8, x=120, y=70, w=30, h=40)
        tied_one = types.SimpleNamespace(class_id=2, score=.8, x=170, y=70, w=30, h=40)
        reports, _ = self.run_main(
            [(7, None)]*50, detections=[[narrow_two, three, tied_one]]*50,
            labels=["two", "three", "one"], expected_numbers=["3"]*50,
        )
        # two 修正为 one 后分数 .765，three 与后一目标同分时取先出现的 three。
        self.assertEqual((reports[-1]["cx"], reports[-1]["cy"]), (192, 96))

    def test_mode7_missing_or_invalid_results_show_dash(self):
        one = types.SimpleNamespace(class_id=0, score=.9, x=70, y=70, w=20, h=40)
        unknown = types.SimpleNamespace(class_id=1, score=.99, x=70, y=70, w=20, h=40)
        self.run_main(
            [(7, None)]*3, detections=[[one], [], [unknown]],
            labels=["one", "background"], expected_numbers=["1", "--", "--"],
        )

    def test_mode7_inference_error_does_not_display_cached_digit(self):
        one = types.SimpleNamespace(class_id=0, score=.9, x=70, y=70, w=20, h=40)
        results = [[one]]*49+[RuntimeError("simulated inference error")]
        reports, packets = self.run_main(
            [(7, None)]*50, detections=results, expected_numbers=["1"]*49+["--"],
        )
        # 原定位异常缓存策略保持不变，但 NUM 明确不伪装成本帧识别成功。
        self.assertIsNotNone(reports[-1])
        self.assertEqual(packets[-1][1], 1)

    def test_mode7_model_unavailable_shows_dash(self):
        reports, packets = self.run_main(
            [(7, None)]*10, expected_numbers=["--"]*10, model_load_error=True,
        )
        self.assertTrue(all(r is None for r in reports))
        self.assertEqual(packets, [SerialLink.encode_result(False)])

    def test_mode7_exit_and_reentry_does_not_keep_display_number(self):
        one = types.SimpleNamespace(class_id=0, score=.9, x=70, y=70, w=20, h=40)
        self.run_main(
            [(7, None), (1, target()), (0, None), (7, None)],
            detections=[[one], [], [], []], expected_numbers=["1", "--", "--", "--"],
        )


if __name__ == "__main__":
    unittest.main()
