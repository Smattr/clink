#!/usr/bin/env python3

"""
Generate contents of a version.c.
"""

import os
from pathlib import Path
import re
import shutil
import subprocess as sp
import sys
from typing import Optional

def last_release() -> str:
  """
  The version of the last release. This will be used as the version number if no
  Git information is available.
  """
  with open(Path(__file__).parent / "../../CHANGELOG.rst", "rt") as f:
    for line in f:
      m = re.match(r"(v\d{4}\.\d{2}\.\d{2})$", line)
      if m is not None:
        return m.group(1)

  return "<unknown>"

def has_git() -> bool:
  """
  Return True if we are in a Git repository and have Git.
  """

  # return False if we don't have Git
  if shutil.which("git") is None:
    return False

  # return False if we have no Git repository information
  if not (Path(__file__).parent / "../../.git").exists():
    return False

  return True

def get_tag() -> Optional[str]:
  """
  Find the version tag of the current Git commit, e.g. v2020.05.03, if it
  exists.
  """
  try:
    tag = sp.check_output(["git", "describe", "--tags"], stderr=sp.DEVNULL)
  except sp.CalledProcessError:
    tag = None

  if tag is not None:
    tag = tag.decode("utf-8", "replace").strip()
    if re.match(r"v[\d\.]+$", tag) is None:
      # not a version tag
      tag = None

  return tag

def get_sha() -> str:
  """
  Find the hash of the current Git commit.
  """
  rev = sp.check_output(["git", "rev-parse", "--verify", "HEAD"])
  rev = rev.decode("utf-8", "replace").strip()

  return rev

def is_dirty() -> bool:
  """
  Determine whether the current working directory has uncommitted changes.
  """
  dirty = False

  p = sp.run(["git", "diff", "--exit-code"], stdout=sp.DEVNULL,
    stderr=sp.DEVNULL)
  dirty |= p.returncode != 0

  p = sp.run(["git", "diff", "--cached", "--exit-code"], stdout=sp.DEVNULL,
    stderr=sp.DEVNULL)
  dirty |= p.returncode != 0

  return dirty

def main(args: [str]) -> int:

  if len(args) != 2 or args[1] == "--help":
    sys.stderr.write(
      f"usage: {args[0]} file\n"
       " write version information as a C source file\n")
    return -1

  # get the contents of the old version file if it exists
  old = None
  if os.path.exists(args[1]):
    with open(args[1], "rt") as f:
      old = f.read()

  version = None

  # look for a version tag on the current commit
  if version is None and has_git():
    tag = get_tag()
    if tag is not None:
      version = f'{tag}{" (dirty)" if is_dirty() else ""}'

  # look for the commit hash as the version
  if version is None and has_git():
    rev = get_sha()
    assert rev is not None
    version = f'Git commit {rev}{" (dirty)" if is_dirty() else ""}'

  # fall back to our known release version
  if version is None:
    version = last_release()

  new =  '#include <clink/version.h>\n' \
         '\n' \
         'const char *clink_version(void) {\n' \
        f'  return "{version}";\n' \
         '}'

  # If the version has changed, update the output. Otherwise we leave the old
  # contents – and more importantly, the timestamp – intact.
  if old != new:
    with open(args[1], "wt") as f:
      f.write(new)

  return 0

if __name__ == "__main__":
  sys.exit(main(sys.argv))
