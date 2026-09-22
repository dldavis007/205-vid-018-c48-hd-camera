"""
gen_unit_test_docs.py

Scans the Unity test suite source files, then:
  1. Generates 205-VID-018 Unit Test Specification.docx  (unfilled checklist)
  2. Runs the test executables
  3. Generates 205-VID-018 Unit Test Report YYYY-MM-DD.docx  (PASS/FAIL results)

Run from any directory:
    python "Documentation/Test_Specification/gen_unit_test_docs.py"
"""

import re
import subprocess
from datetime import date, datetime
from pathlib import Path

from docx import Document
from docx.shared import Pt, Inches, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

# ── Paths ──────────────────────────────────────────────────────────────────
SCRIPT_DIR = Path(__file__).parent.resolve()
DOC_ROOT   = SCRIPT_DIR.parent                          # Documentation/
TESTS_DIR  = DOC_ROOT.parent / 'tests'
REPORT_DIR = DOC_ROOT.parent / 'Documentation' / 'Test_Reports'

# ── Suite definitions ──────────────────────────────────────────────────────
SUITES = [
    {
        'file':    'test_eeprom.c',
        'exe':     'test_eeprom.exe',
        'display': 'EEPROM Emulation',
        'prefix':  'TC-EE',
    },
    {
        'file':    'test_menu_var.c',
        'exe':     'test_menu_var.exe',
        'display': 'Menu Variables',
        'prefix':  'TC-MV',
    },
    {
        'file':    'test_subroutines.c',
        'exe':     'test_subroutines.exe',
        'display': 'Save/Load Subroutines',
        'prefix':  'TC-SB',
    },
]

# ── Style constants ────────────────────────────────────────────────────────
DARK_BLUE  = (26,  58,  92)
LIGHT_BLUE = 'E8F0FE'
HDR_BG     = '1A3A5C'
GREEN      = (0,  128,   0)
RED        = (192,  0,   0)
GREY       = (128, 128, 128)
CHECKBOX   = '☐'

# ── Name constants ────────────────────────────────────────────────────────
PRODUCT_NAME = '205-VID-018 HD Camera'
REVISION     = '1.01'

# ── Low-level helpers ──────────────────────────────────────────────────────

def set_font(run, name='Calibri', size=11, bold=False, italic=False, color=None):
    run.font.name   = name
    run.font.size   = Pt(size)
    run.font.bold   = bold
    run.font.italic = italic
    if color:
        run.font.color.rgb = RGBColor(*color)


def set_cell_shading(cell, hex_color: str):
    tcPr = cell._tc.get_or_add_tcPr()
    shd  = OxmlElement('w:shd')
    shd.set(qn('w:val'),   'clear')
    shd.set(qn('w:color'), 'auto')
    shd.set(qn('w:fill'),  hex_color)
    tcPr.append(shd)


def remove_table_borders(table):
    for row in table.rows:
        for cell in row.cells:
            tcPr  = cell._tc.get_or_add_tcPr()
            tcBdr = OxmlElement('w:tcBorders')
            for side in ('top', 'left', 'bottom', 'right', 'insideH', 'insideV'):
                el = OxmlElement(f'w:{side}')
                el.set(qn('w:val'), 'none')
                tcBdr.append(el)
            tcPr.append(tcBdr)


def humanize(func_name: str) -> str:
    """'test_init_formats_blank_flash' → 'Init formats blank flash'"""
    name = re.sub(r'^test_', '', func_name)
    words = name.replace('_', ' ')
    return words[0].upper() + words[1:] if words else words


# ── Step 1: Parse test source files ───────────────────────────────────────

def _parse_tc_descriptions(content: str) -> dict[str, str]:
    """
    Finds per-test comment blocks:
        /* ====...
         * TC-XX-NN
         * First description line here
    Returns {tc_id: description_text}.
    """
    pattern = re.compile(
        r'/\*\s*=+\s*\n'                      # /* ===... line
        r'\s*\*\s*(TC-[A-Z]+-\d+)\s*\n'       #  * TC-XX-NN
        r'\s*\*\s*([^\n*]+)'                   #  * Description
    )
    return {
        m.group(1).strip(): m.group(2).strip().rstrip(':')
        for m in pattern.finditer(content)
    }


def extract_tests(suite: dict) -> list[tuple[str, str, str]]:
    """
    Returns [(tc_id, func_name, description), …] for every test in the suite.
    TC-ID and func_name are read from the file header comment mapping block.
    Description is pulled from the per-test block comment.
    Falls back to RUN_TEST() call order when the header block is absent.
    """
    content  = (TESTS_DIR / suite['file']).read_text(encoding='utf-8')
    desc_map = _parse_tc_descriptions(content)

    header_re = re.compile(
        rf'({re.escape(suite["prefix"])}-\d+)\s+(test_\w+)'
    )
    pairs = header_re.findall(content)

    if not pairs:
        run_tests = re.findall(r'RUN_TEST\((\w+)\)', content)
        pairs = [(f'{suite["prefix"]}-{i+1:02d}', name)
                 for i, name in enumerate(run_tests)]

    return [(tc_id, func_name, desc_map.get(tc_id, ''))
            for tc_id, func_name in pairs]


# ── Step 2: Run tests, parse Unity output ─────────────────────────────────

def run_suite(suite: dict) -> dict[str, str]:
    """
    Executes the test binary and parses Unity's stdout.
    Unity line format:  filename.c:line:func_name:PASS
                        filename.c:line:func_name:FAIL:message
    Returns {func_name: 'PASS'|'FAIL'}.
    """
    exe     = TESTS_DIR / suite['exe']
    results: dict[str, str] = {}

    try:
        proc = subprocess.run(
            [str(exe)],
            capture_output=True, text=True,
            cwd=str(TESTS_DIR), timeout=30,
        )
        output = proc.stdout + proc.stderr
    except (FileNotFoundError, subprocess.TimeoutExpired) as exc:
        print(f'  [!] {suite["exe"]}: {exc}')
        return results

    for line in output.splitlines():
        m = re.match(r'^[^:]+:\d+:(\w+):(PASS|FAIL)', line)
        if m:
            results[m.group(1)] = m.group(2)

    return results


# ── Step 3: Shared document components ────────────────────────────────────

def _add_title_block(doc: Document, subtitle: str, sw_rev: str):
    for section in doc.sections:
        section.top_margin    = Inches(1)
        section.bottom_margin = Inches(1)
        section.left_margin   = Inches(1)
        section.right_margin  = Inches(1)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run(PRODUCT_NAME)
    set_font(r, size=16, bold=True, color=DARK_BLUE)

    doc.add_paragraph()

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run(subtitle)
    set_font(r, size=13, color=DARK_BLUE)

    doc.add_paragraph()

    hdr = doc.add_table(rows=0, cols=4)
    hdr.style  = 'Table Grid'
    hdr.autofit = True

    def _hdr_row(l1, v1, l2='', v2=''):
        row = hdr.add_row()
        for i, (cell, txt) in enumerate(zip(row.cells, [l1, v1, l2, v2])):
            cell.text = txt
            for para in cell.paragraphs:
                for run in para.runs:
                    run.font.bold = (i % 2 == 0)
                    run.font.size = Pt(10)

    _hdr_row('Tester:', '_________________________', 'Date:', str(date.today()))
    _hdr_row('Unit S/N:', '_________________________', 'SW Rev:', sw_rev)
    remove_table_borders(hdr)
    doc.add_paragraph()


def _create_results_table(doc: Document):
    """4-column table: indicator | test | result | notes."""
    tbl  = doc.add_table(rows=1, cols=4)
    tbl.style = 'Table Grid'

    widths  = [Inches(0.35), Inches(4.05), Inches(0.75), Inches(1.35)]
    headers = [CHECKBOX, 'Test', 'Result', 'Notes']

    for i, (cell, txt) in enumerate(zip(tbl.rows[0].cells, headers)):
        cell.width = widths[i]
        para = cell.paragraphs[0]
        para.alignment = (WD_ALIGN_PARAGRAPH.CENTER if i != 1
                          else WD_ALIGN_PARAGRAPH.LEFT)
        r = para.add_run(txt)
        set_font(r, size=10, bold=True, color=(255, 255, 255))
        set_cell_shading(cell, HDR_BG)

    return tbl


def _add_section_row(tbl, suite: dict):
    row  = tbl.add_row()
    cell = row.cells[0]
    cell.merge(row.cells[3])
    r = cell.paragraphs[0].add_run(f'{suite["prefix"]} — {suite["display"]}')
    set_font(r, size=10, bold=True, color=DARK_BLUE)
    set_cell_shading(cell, LIGHT_BLUE)


def _fill_name_cell(cell, tc_id: str, func_name: str, description: str):
    """Bold TC ID + name on line 1; small italic description on line 2."""
    para = cell.paragraphs[0]
    r = para.add_run(f'{tc_id}: {humanize(func_name)}')
    set_font(r, size=10, bold=True)
    if description:
        p = cell.add_paragraph()
        r = p.add_run(description)
        set_font(r, size=9, italic=True, color=GREY)


# ── Step 4: Build Test Specification (unfilled) ────────────────────────────

def build_spec(suites_tests: list) -> Path:
    doc = Document()
    _add_title_block(doc, f'Unit Test Specification — Rev {{ {REVISION} }}', str(REVISION))
    tbl = _create_results_table(doc)

    for suite, tests in suites_tests:
        _add_section_row(tbl, suite)
        for tc_id, func_name, description in tests:
            row   = tbl.add_row()
            cells = row.cells

            cb = cells[0].paragraphs[0]
            cb.alignment = WD_ALIGN_PARAGRAPH.CENTER
            r = cb.add_run(CHECKBOX)
            r.font.name = 'Segoe UI Symbol'
            r.font.size = Pt(12)

            _fill_name_cell(cells[1], tc_id, func_name, description)

            res = cells[2].paragraphs[0]
            res.alignment = WD_ALIGN_PARAGRAPH.CENTER
            r = res.add_run('☐ Pass\n☐ Fail\n☐ Skip')
            r.font.name = 'Segoe UI Symbol'
            r.font.size = Pt(9)

            cells[3].paragraphs[0].add_run('')

    doc.add_paragraph()
    sig = doc.add_paragraph()
    r = sig.add_run(
        'Overall Result:  ☐ PASS   ☐ FAIL          '
        'Signature: _______________________   Date: ____________'
    )
    r.font.name = 'Segoe UI Symbol'
    set_font(r, size=11)

    out = SCRIPT_DIR / '205-VID-018 Unit Test Specification.docx'
    doc.save(str(out))
    return out


# ── Step 5: Build Test Report (filled with PASS/FAIL) ─────────────────────

def build_report(suites_tests: list, all_results: list) -> tuple[int, int, int, Path]:
    doc = Document()
    _add_title_block(doc, f'Unit Test Report — {REVISION} ({date.today()})', REVISION)
    tbl = _create_results_table(doc)

    total = passed = failed = 0

    sym_map = {'PASS': '✔', 'FAIL': '✘'}
    col_map = {'PASS': GREEN, 'FAIL': RED}

    for (suite, tests), results in zip(suites_tests, all_results):
        _add_section_row(tbl, suite)
        for tc_id, func_name, description in tests:
            verdict = results.get(func_name, 'NOT RUN')
            row     = tbl.add_row()
            cells   = row.cells

            cb = cells[0].paragraphs[0]
            cb.alignment = WD_ALIGN_PARAGRAPH.CENTER
            r = cb.add_run(sym_map.get(verdict, '?'))
            r.font.name = 'Segoe UI Symbol'
            r.font.size = Pt(12)
            r.font.color.rgb = RGBColor(*col_map.get(verdict, GREY))

            _fill_name_cell(cells[1], tc_id, func_name, description)

            res = cells[2].paragraphs[0]
            res.alignment = WD_ALIGN_PARAGRAPH.CENTER
            r = res.add_run(verdict)
            set_font(r, size=10, bold=(verdict in ('PASS', 'FAIL')),
                     color=col_map.get(verdict, GREY))

            cells[3].paragraphs[0].add_run('')

            total += 1
            if verdict == 'PASS':
                passed += 1
            elif verdict == 'FAIL':
                failed += 1

    overall       = 'PASS' if (failed == 0 and passed == total and total > 0) else 'FAIL'
    overall_color = GREEN if overall == 'PASS' else RED
    final_color = GREY

    doc.add_paragraph()
    p = doc.add_paragraph()
    r = p.add_run(f'Summary: {passed}/{total} passed — {failed} failed')
    set_font(r, size=12, bold=True, color=overall_color)

    doc.add_paragraph()
    sig = doc.add_paragraph()
    r = sig.add_run(
        f'Overall: {overall}          '
        'Tested by: _______________________   '
        'Signature: _______________________   Date: ____________'
    )
    set_font(r, size=11, bold=True, color=final_color)

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    out = REPORT_DIR / f'{PRODUCT_NAME} Unit Test Report {datetime.now().strftime("%Y-%m-%d %H%M")}.docx'
    doc.save(str(out))
    return passed, failed, total, out


# ── Main ───────────────────────────────────────────────────────────────────

def main():
    print('Scanning test files …')
    suites_tests = []
    for suite in SUITES:
        tests = extract_tests(suite)
        print(f'  {suite["file"]}: {len(tests)} tests')
        suites_tests.append((suite, tests))

    print()
    spec_path = build_spec(suites_tests)
    print(f'Spec saved:   {spec_path}')

    print('\nRunning tests …')
    all_results = []
    for suite, _ in suites_tests:
        print(f'  {suite["exe"]} … ', end='', flush=True)
        results = run_suite(suite)
        all_results.append(results)
        n_pass = sum(1 for v in results.values() if v == 'PASS')
        n_fail = sum(1 for v in results.values() if v == 'FAIL')
        print(f'{n_pass} PASS, {n_fail} FAIL')

    passed, failed, total, report_path = build_report(suites_tests, all_results)
    print(f'\nReport saved: {report_path}')
    print(f'\nResult: {passed}/{total} passed, {failed} failed.')


if __name__ == '__main__':
    main()
