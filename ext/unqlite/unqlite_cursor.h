#ifndef _unqlite_cursor_h
#define _unqlite_cursor_h

#include <unqlite_ruby.h>

void Init_unqlite_cursor();
VALUE unqlite_cursor_release(VALUE self);

typedef struct
{
  unqlite_kv_cursor* cursor;
  VALUE rb_database;
} unqliteRubyCursor;

#endif /* _unqlite_cursor_h */
