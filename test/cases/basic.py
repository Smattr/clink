# basic Python test

def foo(x: int):
  return x + 1

class Bar():
  def __init__():
    pass

# RUN: clink --build-only --database={%t} --debug {%s} >/dev/null

# RUN: echo "select records.path, symbols.category, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'foo' and symbols.line < 12;" | sqlite3 {%t}
# CHECK: {%s}|0|3|5

# RUN: echo "select records.path, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'x' and symbols.line < 12 order by symbols.line;" | sqlite3 {%t}
# CHECK: {%s}|3|9
# CHECK: {%s}|4|10

# RUN: echo "select symbols.name, records.path, symbols.category, symbols.line, symbols.col from symbols inner join records on symbols.path = records.id where symbols.name = 'Bar' and symbols.line < 12;" | sqlite3 {%t}
# CHECK: Bar|{%s}|0|6|7
