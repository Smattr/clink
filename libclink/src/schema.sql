create table if not exists symbols
  /* symbols found within source files */
(
  name text not null,
  path integer not null,
  category integer not null,
  line integer not null,
  col integer not null,
  start_line integer not null,
  start_col integer not null,
  start_byte integer not null,
  end_line integer not null,
  end_col integer not null,
  end_byte integer not null,
  parent text,
  unique(name, path, category, line, col),
  foreign key(path) references records(id)
);

create table if not exists content
  /* lines of ANSI-colour-enriched source code text */
(
  path integer not null,
  line integer not null,
  body text not null,
  unique(path, line),
  foreign key(path) references records(id)
);

create table if not exists records
  /* paths to source code files that were scanned */
(
  id integer primary key,
  path text not null unique,
  hash integer not null,
  timestamp integer not null
);
