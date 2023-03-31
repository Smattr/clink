#!/usr/bin/env python3

"""
Generate contents of a schema.c.
"""

import hashlib
import io
import sys
from pathlib import Path
from typing import Iterator


def get_hash(sql: Path) -> str:
    """
    calculate a digest of a given file
    """
    return hashlib.sha256(sql.read_bytes()).hexdigest()


def get_statements(sql: Path) -> Iterator[str]:
    """
    read ;-delimited SQL statements from a file
    """
    accrued = io.StringIO()
    last_was_space = False
    with open(sql, "rt", encoding="utf-8") as f:
        while True:
            c = f.read(1)

            if c == "":
                break

            # de-dupe and normalise spaces
            if c.isspace():
                if not last_was_space and len(accrued.getvalue()) > 0:
                    accrued.write(" ")
                last_was_space = True
                continue
            last_was_space = False

            accrued.write(c)

            # are we at the end of a statement?
            if c == ";":
                yield accrued.getvalue()
                accrued = io.StringIO()


def main(args: [str]) -> int:
    """
    entry point
    """

    if len(args) != 2 or args[1] == "--help":
        sys.stderr.write(
            f"usage: {args[0]} file\n write database schema as a C source file\n"
        )
        return -1

    # we expect the schema source to exist adjacent to us
    schema_sql = Path(__file__).parent / "schema.sql"
    assert schema_sql.exists(), "database schema missing"

    # construct a source defining the database schema
    schema_c = io.StringIO()
    schema_c.write(
        '#include "schema.h"\n#include <stddef.h>\n\nconst char *SCHEMA[] = {\n'
    )
    statement_count = 0
    for stmt in get_statements(schema_sql):
        schema_c.write(f'  "{stmt}",\n')
        statement_count += 1

    # write a version of the schema itself
    version = get_hash(schema_sql)
    schema_c.write(
        f'  "insert into metadata (key, value) values (\\"schema_version\\", \\"{version}\\");",\n'
    )
    statement_count += 1

    schema_c.write(f"}};\n\nconst size_t SCHEMA_LENGTH = {statement_count};\n\n")

    # write a way to access the schema version
    schema_c.write(f'const char *schema_version(void) {{\n  return "{version}";\n}}\n')

    # write out schema.c
    Path(args[1]).write_text(schema_c.getvalue(), encoding="utf-8")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
