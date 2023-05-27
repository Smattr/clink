create table if not exists symbols (
  name text not null,
  path integer not null,
  category integer not null,
  line integer not null,
  col integer not null,
  parent text,
  unique(name, path, category, line, col),
  foreign key(path) references records(id));

create table if not exists content (
  path integer not null,
  line integer not null,
  body text not null,
  unique(path, line),
  foreign key(path) references records(id));

create table if not exists records (
  id integer primary key,
  path text not null unique,
  hash integer not null,
  timestamp integer not null);

create table if not exists metadata (
  key text not null unique,
  value text not null);
