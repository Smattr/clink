"""
Clink integration test suite
"""

import pytest
import re
import sqlite3
import subprocess
from pathlib import Path
from typing import Iterator, Tuple

def records(db: Path) -> Iterator[str]:
  """
  yield all Clink records from a given database
  """
  connection = sqlite3.connect(db)
  cursor = connection.execute("select * from symbols")

  while True:

    record = cursor.fetchone()
    if record is None:
      break

    name, path, typ, lineno, colno, content = record

    path = Path(path).name

    if typ == 0:
      typ = "DEFINITION"
    elif typ == 1:
      typ = "FUNCTION_CALL"
    elif typ == 2:
      typ = "REFERENCE"
    elif typ == 3:
      typ = "INCLUDE"
    else:
      raise ValueError(f"unsupported symbol type {typ}")

    yield f"{name}|{path}|{typ}|{lineno}|{colno}|{content}"

# find our associated test cases
root = Path(__file__).parent / "cases"
cases = list(x.name for x in root.iterdir())

@pytest.mark.parametrize("test_case", cases)
def test_case(tmp_path: Path, test_case: str):
  """
  run Clink on the given test case and validate its CHECK lines
  """

  src = Path(__file__).resolve().parent / "cases" / test_case
  db = tmp_path / "clink.db"

  # run Clink, parsing the given file
  subprocess.check_call(["clink", "--build-only", "--database", db, src])

  # read the contents of the database back in
  symbols = list(records(db))

  # scan the test case for CHECK lines
  with open(src, "rt", encoding="utf-8") as f:
    for line in f:

      m = re.match(f"^\s*//\s*CHECK:\s*(.*)$", line)
      if m is None:
        continue

      assert m.group(1) in symbols, f"no match for {m.group(0)}"
