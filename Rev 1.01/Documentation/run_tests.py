"""
205-VID-018 Interactive Functional Test Runner

NOT a pytest script — this is a standalone interactive runner for hardware tests.
Run it from the command line while physically performing each test step.
At the end it generates a timestamped HTML report you can save with the documentation.

Usage:  python run_tests.py
Output: test_report_YYYY-MM-DD_HHMMSS.html  (written to the same folder)
"""

import datetime
import html as html_lib
import os
import sys

# ──────────────────────────────────────────────────────────────────────────────
# Test case definitions — edit this list to add / remove / reorder tests
# ──────────────────────────────────────────────────────────────────────────────
TEST_CASES = [
    {
        "id": "TC-001",
        "title": "Build Verification",
        "setup": "Open the CM4 project in STM32CubeIDE.",
        "items": [
            "STM32CubeIDE project opens without errors",
            "CM4 target builds with 0 errors, 0 blocking warnings",
            "Output .elf / .bin is generated successfully",
        ],
    },
    {
        "id": "TC-002",
        "title": "Power-On and CAN Communication",
        "setup": "Connect DUT to button box and power supply per wiring diagram. Do not power on yet.",
        "items": [
            "Device powers on at 24 VDC; current draw ≤ 400 mA",
            "Controller (button box) recognizes CANopen node 0x21 within 30 seconds",
            "Device reaches Operational state (no NMT error observed)",
        ],
    },
    {
        "id": "TC-003",
        "title": "Menu Navigation",
        "setup": "Device is powered and in normal operation. Controller is connected.",
        "items": [
            "Menu button enters the main menu",
            "Up / Down cursor moves through menu items",
            "CAMERAS → HD CAM submenu is reachable from main menu",
            "SELECT enters a submenu; Back / Exit returns to parent",
            "Menu does not lock up or wrap incorrectly at list boundaries",
        ],
    },
    {
        "id": "TC-004",
        "title": "Read-Only Variables Cannot Be Edited",
        "setup": "Navigate to HD CAM → STATUS MENU.",
        "items": [
            "Status Menu shows Software Revision — cursor cannot enter edit mode on this field",
            "Status Menu shows Camera Address (hex) — field is display-only, no edit mode",
            "Status Menu shows Serial Number — field is display-only, no edit mode",
        ],
    },
    {
        "id": "TC-005",
        "title": "String Variable Editing (Camera Tag)",
        "setup": "Navigate to HD CAM → SETTINGS.",
        "items": [
            "SETTINGS → Camera Tag enters edit mode",
            "Inc / Dec buttons change the active character",
            "Cursor advances and retreats between character positions",
            "Exiting edit mode retains the changed value on screen",
        ],
    },
    {
        "id": "TC-006",
        "title": "Enum Variable Editing (Microphone ON/OFF)",
        "setup": "Navigate to HD CAM → SETTINGS. Connect speaker.",
        "items": [
            "SETTINGS → MICROPHONE shows current state (ON or OFF)",
            "SELECT toggles OFF → ON; audio output confirmed from speaker",
            "SELECT toggles ON → OFF; no audio output confirmed from speaker",
        ],
    },
    {
        "id": "TC-007",
        "title": "Save Variables (EEPROM Write on Menu Exit)",
        "setup": "Begin with known baseline values for Camera Tag and Microphone.",
        "items": [
            "Change Camera Tag to a non-default value, then fully exit menu",
            "Power-cycle device; Camera Tag is the changed value (not default)",
            "Change Microphone to non-default value, then fully exit menu",
            "Power-cycle device; Microphone is the changed value",
        ],
    },
    {
        "id": "TC-008",
        "title": "Load Variables (EEPROM Read on Boot)",
        "setup": "TC-007 must be completed first with known values saved.",
        "items": [
            "With Camera Tag and Microphone previously saved, power-cycle device",
            "Camera Tag restores correctly within 30-second init window",
            "Microphone setting restores correctly within 30-second init window",
        ],
    },
    {
        "id": "TC-009",
        "title": "Restore Defaults",
        "setup": "Navigate to HD CAM → DEFAULTS.",
        "items": [
            "DEFAULTS → RESTORE DEFAULTS executes without crash",
            "Camera Tag returns to factory default value after restore",
            "Microphone returns to factory default value after restore",
            "Power-cycle; default values persist (restore write reaches EEPROM)",
        ],
    },
    {
        "id": "TC-010",
        "title": "Camera OSD Hardware Integration",
        "setup": "Connect BNC video to HD-TVI monitor. Device is powered and operational.",
        "items": [
            "F1 button enters camera functions",
            "Menu button opens HD Camera OSD on monitor",
            "Slow / Rotate CW navigate OSD selection; Down / Up adjust value; Menu acts as Enter",
            "Set a known OSD setting (e.g., PRESET = INDOOR); exit OSD and camera functions",
            "Power-cycle; camera OSD setting is retained (PRESET still = INDOOR on re-entry)",
        ],
    },
    {
        "id": "TC-011",
        "title": "Watchdog Stability — Normal Operation",
        "setup": "Device is powered and running normally. Monitor for resets.",
        "items": [
            "Device operates continuously ≥ 5 minutes with no unexpected reset",
            "No WWDG2 or TPS3850 hardware watchdog resets observed during normal use",
        ],
    },
]

# ──────────────────────────────────────────────────────────────────────────────

DIVIDER = '─' * 62
RESULT_MAP = {'P': 'PASS', 'F': 'FAIL', 'S': 'SKIP'}
VALID = set(RESULT_MAP.keys())


def prompt_result(prompt_text):
    while True:
        ans = input(f'     [{prompt_text}]  P / F / S: ').strip().upper()
        if ans in VALID:
            return ans
        print('     Please enter P (pass), F (fail), or S (skip).')


def run_tests():
    print()
    print('=' * 62)
    print('  205-VID-018 C48 HD Camera — Functional Test Runner')
    print('=' * 62)
    print()
    tester  = input('Tester name  : ').strip() or 'Unknown'
    unit_sn = input('Unit S/N     : ').strip() or 'N/A'
    sw_rev  = input('SW Rev [1.01]: ').strip() or '1.01'
    print()

    session_results = []
    total = passed = failed = skipped = 0

    for tc in TEST_CASES:
        print()
        print(DIVIDER)
        print(f"  {tc['id']} — {tc['title']}")
        print(DIVIDER)
        if tc.get('setup'):
            print(f"  Setup: {tc['setup']}")
        print()

        tc_record = {'id': tc['id'], 'title': tc['title'], 'items': []}

        for item in tc['items']:
            print(f"  ☐  {item}")
            result = prompt_result(item[:40])
            note = ''
            if result == 'F':
                note = input('     Failure notes (required): ').strip()
            elif result == 'S':
                note = input('     Skip reason: ').strip()

            tc_record['items'].append({'text': item, 'result': result, 'note': note})
            total += 1
            if result == 'P':
                passed += 1
            elif result == 'F':
                failed += 1
            else:
                skipped += 1
            print()

        session_results.append(tc_record)

    overall = 'PASS' if failed == 0 else 'FAIL'
    report_path = generate_html_report(
        tester, unit_sn, sw_rev,
        session_results, total, passed, failed, skipped, overall
    )

    print()
    print('=' * 62)
    print(f'  Overall: {overall}   ({passed} pass, {failed} fail, {skipped} skip / {total} total)')
    print(f'  Report : {report_path}')
    print('=' * 62)
    print()


def generate_html_report(tester, unit_sn, sw_rev,
                          results, total, passed, failed, skipped, overall):
    now = datetime.datetime.now()
    date_str = now.strftime('%Y-%m-%d')
    timestamp = now.strftime('%Y-%m-%d %H:%M:%S')
    file_ts = now.strftime('%Y-%m-%d_%H%M%S')

    pass_color  = '#155724'
    fail_color  = '#721c24'
    skip_color  = '#856404'
    overall_bg  = '#d4edda' if overall == 'PASS' else '#f8d7da'
    overall_fg  = pass_color if overall == 'PASS' else fail_color

    def e(s):
        return html_lib.escape(str(s))

    rows_html = ''
    row_num = 1
    for tc in results:
        # TC section header row
        rows_html += f'''
      <tr class="tc-header">
        <td colspan="5">{e(tc["id"])} — {e(tc["title"])}</td>
      </tr>'''
        for item in tc['items']:
            r = item['result']
            cls = {'P': 'pass-cell', 'F': 'fail-cell', 'S': 'skip-cell'}[r]
            color = {'P': pass_color, 'F': fail_color, 'S': skip_color}[r]
            rows_html += f'''
      <tr>
        <td style="text-align:center">{row_num}</td>
        <td>{e(tc["id"])}</td>
        <td>{e(item["text"])}</td>
        <td class="{cls}" style="color:{color};font-weight:bold;text-align:center">{RESULT_MAP[r]}</td>
        <td>{e(item["note"])}</td>
      </tr>'''
            row_num += 1

    html_out = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>205-VID-018 Test Report — {date_str}</title>
  <style>
    body  {{ font-family: Calibri, Arial, sans-serif; margin: 40px; color: #222; }}
    h1    {{ color: #1a3a5c; margin-bottom: 4px; }}
    h2    {{ color: #1a3a5c; border-bottom: 2px solid #1a3a5c; padding-bottom: 4px; }}
    .meta {{ border-collapse: collapse; margin-bottom: 18px; }}
    .meta td {{ padding: 3px 12px 3px 0; font-size: 10.5pt; }}
    .meta .lbl {{ font-weight: bold; color: #1a3a5c; }}
    .summary {{
      background: {overall_bg};
      border-radius: 6px;
      padding: 14px 20px;
      margin-bottom: 24px;
      display: inline-block;
    }}
    .overall {{ font-size: 22pt; font-weight: bold; color: {overall_fg}; }}
    .counts  {{ font-size: 11pt; margin-top: 4px; }}
    table.results {{ border-collapse: collapse; width: 100%; font-size: 10pt; }}
    table.results th {{
      background: #1a3a5c; color: white;
      padding: 7px 10px; text-align: left;
    }}
    table.results td {{ padding: 5px 10px; border: 1px solid #ddd; vertical-align: top; }}
    table.results tr:nth-child(even):not(.tc-header) {{ background: #f7f9fc; }}
    .tc-header td {{
      background: #e8f0fe; font-weight: bold;
      color: #1a3a5c; padding: 6px 10px;
    }}
    .pass-cell {{ background: #f0fff4; }}
    .fail-cell {{ background: #fff5f5; }}
    .skip-cell {{ background: #fffbea; }}
    .footer {{ margin-top: 30px; font-size: 9pt; color: #888; }}
    @media print {{
      body {{ margin: 20px; }}
      .summary {{ -webkit-print-color-adjust: exact; print-color-adjust: exact; }}
    }}
  </style>
</head>
<body>
  <h1>205-VID-018 C48 HD Camera</h1>
  <h2>Functional Test Report</h2>

  <table class="meta">
    <tr><td class="lbl">Tester:</td><td>{e(tester)}</td>
        <td class="lbl">Date:</td><td>{date_str}</td></tr>
    <tr><td class="lbl">Unit S/N:</td><td>{e(unit_sn)}</td>
        <td class="lbl">SW Revision:</td><td>{e(sw_rev)}</td></tr>
    <tr><td class="lbl">Report generated:</td><td colspan="3">{timestamp}</td></tr>
  </table>

  <div class="summary">
    <div class="overall">Overall: {overall}</div>
    <div class="counts">
      Pass: <b>{passed}</b> &nbsp;|&nbsp;
      Fail: <b>{failed}</b> &nbsp;|&nbsp;
      Skip: <b>{skipped}</b> &nbsp;|&nbsp;
      Total: <b>{total}</b>
    </div>
  </div>

  <h2>Detailed Results</h2>
  <table class="results">
    <thead>
      <tr>
        <th style="width:3%">#</th>
        <th style="width:9%">TC ID</th>
        <th style="width:52%">Check Item</th>
        <th style="width:8%">Result</th>
        <th style="width:28%">Notes</th>
      </tr>
    </thead>
    <tbody>{rows_html}
    </tbody>
  </table>

  <div class="footer">
    Generated by 205-VID-018 run_tests.py &nbsp;|&nbsp; {timestamp}
  </div>
</body>
</html>"""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    out_path = os.path.join(script_dir, f'test_report_{file_ts}.html')
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write(html_out)
    return out_path


if __name__ == '__main__':
    try:
        run_tests()
    except KeyboardInterrupt:
        print('\n\nTest run cancelled.')
        sys.exit(1)
