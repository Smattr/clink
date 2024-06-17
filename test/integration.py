"""
Clink integration test suite
"""

import os  # pylint: disable=unused-import
import re
import subprocess
from pathlib import Path

import pytest
from packaging import version  # pylint: disable=unused-import


def is_python(path: Path) -> bool:
    """
    is this a path to a Python source file?
    """
    return path.suffix.lower() == ".py"


def lit(tmp: Path, source: Path):
    """
    a minimal implementation of something like the LLVM Integrated Tester (LIT)
    """
    saw_directive = False
    output = ""

    context = {
        "%s": str(source),
        "%S": str(source.parent),
        "%t": str(tmp / "tempfile"),
        "%T": str(tmp),
    }

    xfail = None

    with open(source, "rt", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):

            # is this a LIT line?
            if is_python(source):
                m = re.match(
                    r"\s*#\s*(?P<directive>[A-Z]+)\s*:\s*(?P<content>.*)\s*$", line
                )
            else:
                m = re.match(
                    r"\s*//\s*(?P<directive>[A-Z]+)\s*:\s*(?P<content>.*)\s*$", line
                )
            if m is None:
                continue

            saw_directive = True

            directive = m.group("directive")
            content = m.group("content").format(**context)

            # is this a command to be run?
            if directive == "RUN":
                try:
                    result = subprocess.check_output(
                        ["bash", "-o", "pipefail", "-c", "--", content],
                        stdin=subprocess.DEVNULL,
                        cwd=tmp,
                        universal_newlines=True,
                    )
                except subprocess.CalledProcessError:
                    if xfail is None:
                        raise
                    pytest.xfail(xfail)
                output += result

            # is this a check of previous output?
            elif directive == "CHECK":
                output = output.lstrip()
                try:
                    assert output.startswith(
                        content
                    ), f"{source}:{lineno}: failed CHECK: {content}"
                except AssertionError:
                    if xfail is None:
                        raise
                    pytest.xfail(xfail)
                output = output[len(content) :]

            # is this an indication this test is expected to fail?
            elif directive == "XFAIL":
                if eval(content):
                    xfail = f"{source}:{lineno}: XFAIL marker"

            else:
                pytest.fail(f'unrecognised directive "{directive}"')

    if xfail:
        raise RuntimeError(f"XPASS: {xfail}")

    assert saw_directive, "no directives recognised"


# find our associated test cases
root = Path(__file__).parent / "cases"
cases = sorted(
    x.name
    for x in root.iterdir()
    if x.suffix in (".c", ".def", ".l", ".py", ".td", ".y")
)


@pytest.mark.parametrize("case", cases)
def test_case(tmp_path: Path, case: str):
    """
    run Clink on the given test case and validate its CHECK lines
    """
    lit(tmp_path, Path(__file__).parent / "cases" / case)


def test_243(tmp_path: Path):
    """
    https://github.com/Smattr/clink/issues/243
    Clink should not get confused by compile command argument ordering
    """

    # create a directory structure representative of typical compilation
    src = tmp_path / "src"
    src.mkdir()
    build = tmp_path / "build"
    build.mkdir()

    # create an arbitrary source file
    foo_c = src / "foo.c"
    foo_c.write_text("int x;\n")

    # create a compilation database with command line arguments unusually ordered
    db = build / "compile_commands.json"
    db.write_text(
        f"""\
    [
      {{
        "arguments": [
          "gcc",
          "-c",
          "../src/foo.c",
          "-o",
          "foo.o"
        ],
        "directory": "f{build.absolute()}",
        "file": "{foo_c.absolute()}"
      }}
    ]
    """,
        encoding="utf-8",
    )

    # run Clink on this working directory
    stderr = subprocess.check_output(
        ["clink", "--build-only", "--debug", "--jobs=1"],
        cwd=tmp_path,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )

    assert (
        "no compile_commands.json entry found" not in stderr
    ), "failed to find a compilation entry that exists"
