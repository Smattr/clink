# check that we do not recognise things in comments as symbols

# foo in a comment

# RUN: clink --build-only --database={%t} --debug {%s} >/dev/null
# RUN: echo 'select * from symbols where name = "foo";' | sqlite3 {%t}
# RUN: echo "marker"
# CHECK: marker
