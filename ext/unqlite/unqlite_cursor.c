#include <unqlite_cursor.h>

/*
 * Document-class: UnQLite::Cursor
 *
 * Cursors provide a mechanism by which you can iterate over the
 * records in a database. Using cursors, you can seek, fetch, move,
 * and delete database records.
 */

/* Get cursor context pointer from Ruby object */
#define GetCursor(obj, cursp) {                         \
    Data_Get_Struct((obj), unqliteRubyCursor, (cursp)); \
    if ((cursp) == 0) released_cursor();                \
    if ((cursp)->cursor == 0) released_cursor();        \
  }

/* Get cursor context pointer and native cursor pointer from Ruby object */
#define GetCursor2(obj, cursp, curs) {         \
    GetCursor((obj), (cursp));                \
    (curs) = (cursp)->cursor;                 \
  }

/* Raise error for already released cursor */
static void released_cursor()
{
  rb_raise(rb_eRuntimeError, "Released cursor");
}

/* Wrapped object: mark */
static void unqlite_cursor_mark(unqliteRubyCursor* rcursor)
{
  rb_gc_mark(rcursor->rb_database);
}

/* Wrapped object: deallocate */
static void unqlite_cursor_deallocate(unqliteRubyCursor* rcursor)
{
  int rc;
  if (rcursor->cursor) {
    unqliteRubyPtr rdatabase;
    Data_Get_Struct(rcursor->rb_database, unqliteRuby, rdatabase);
    rc = unqlite_kv_cursor_release(rdatabase->pDb, rcursor->cursor);
    CHECK(0, rc);
  }
  xfree(rcursor);
}

/* Wrapped object: allocate */
VALUE unqlite_cursor_allocate(VALUE klass)
{
  unqliteRubyCursor *rcursor = ALLOC(unqliteRubyCursor);
  rcursor->cursor = 0;
  rcursor->rb_database = Qnil;
  return Data_Wrap_Struct(klass, unqlite_cursor_mark, unqlite_cursor_deallocate, rcursor);
}

/*
 * call-seq:
 *    UnQLite::Cursor.new(unqlite)
 *
 * Initializes a new cursor from an existing database.
 */
static VALUE unqlite_cursor_initialize(VALUE self, VALUE rb_database)
{
  unqliteRubyCursor* rcursor;
  unqliteRuby* rdatabase;
  int rc;
  Data_Get_Struct(self, unqliteRubyCursor, rcursor);
  Data_Get_Struct(rb_database, unqliteRuby, rdatabase);
  rcursor->rb_database = rb_database;
  rb_ary_push(rdatabase->acursors, self);
  rc = unqlite_kv_cursor_init(rdatabase->pDb, &rcursor->cursor);
  CHECK(rdatabase->pDb, rc);
  return self;
}

/*
 * call-seq:
 *    cursor.reset
 *
 * Resets the cursor.
 */
static VALUE unqlite_cursor_reset(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  unqliteRuby* rdatabase;
  int rc;

  GetCursor2(self, rcursor, cursor);
  Data_Get_Struct(rcursor->rb_database, unqliteRuby, rdatabase);
  rc = unqlite_kv_cursor_reset(cursor);
  CHECK(rdatabase->pDb, rc);
  return Qtrue;
}

/*
 * call-seq:
 *    cursor.first!
 *
 * Sets the cursor to point to the first entry in the database.
 */
static VALUE unqlite_cursor_first(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  unqliteRuby* rdatabase;
  int rc;

  GetCursor2(self, rcursor, cursor);
  Data_Get_Struct(rcursor->rb_database, unqliteRuby, rdatabase);
  rc = unqlite_kv_cursor_first_entry(cursor);
  CHECK(rdatabase->pDb, rc);
  return Qtrue;
}

/*
 * call-seq:
 *    cursor.last!
 *
 * Sets the cursor to point to the last entry in the database.
 */
static VALUE unqlite_cursor_last(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  unqliteRuby* rdatabase;
  int rc;

  GetCursor2(self, rcursor, cursor);
  Data_Get_Struct(rcursor->rb_database, unqliteRuby, rdatabase);
  rc = unqlite_kv_cursor_last_entry(cursor);
  CHECK(rdatabase->pDb, rc);
  return Qtrue;
}

/*
 * call-seq:
 *    cursor.valid?
 *
 * Returns true if the cursor points to a valid entry, otherwise false.
 */
static VALUE unqlite_cursor_valid(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;

  GetCursor2(self, rcursor, cursor);
  return unqlite_kv_cursor_valid_entry(cursor) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *    cursor.next!
 *
 * Step cursor forwards to the next entry.
 */
static VALUE unqlite_cursor_next(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  unqliteRuby* rdatabase;
  int rc;

  GetCursor2(self, rcursor, cursor);
  Data_Get_Struct(rcursor->rb_database, unqliteRuby, rdatabase);
  rc = unqlite_kv_cursor_next_entry(cursor);
  CHECK(rdatabase->pDb, rc);
  return Qtrue;
}

/*
 * call-seq:
 *    cursor.prev!
 *
 * Step cursor backwards to the previous entry.
 */
static VALUE unqlite_cursor_prev(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  unqliteRuby* rdatabase;
  int rc;

  GetCursor2(self, rcursor, cursor);
  Data_Get_Struct(rcursor->rb_database, unqliteRuby, rdatabase);
  rc = unqlite_kv_cursor_prev_entry(cursor);
  CHECK(rdatabase->pDb, rc);
  return Qtrue;
}

/*
 * call-seq:
 *    cursor.delete!
 *
 * Delete the entry referenced by the cursor.
 */
static VALUE unqlite_cursor_delete(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  unqliteRuby* rdatabase;
  int rc;

  GetCursor2(self, rcursor, cursor);
  Data_Get_Struct(rcursor->rb_database, unqliteRuby, rdatabase);
  rc = unqlite_kv_cursor_delete_entry(cursor);
  CHECK(rdatabase->pDb, rc);
  return Qtrue;
}

/*
 * call-seq:
 *    cursor.seek(key)
 *    cursor.seek(key, direction)
 *
 * Resets the cursor. Direction can be one of:
 *
 * * +UnQLite::CURSOR_MATCH_EXACT+
 * * +UnQLite::CURSOR_MATCH_LE+
 * * +UnQLite::CURSOR_MATCH_GE+
 *
 * Default direction is exact match.
 */
static VALUE unqlite_cursor_seek(int argc, VALUE* argv, VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  int rc;
  VALUE key, direction;

  GetCursor2(self, rcursor, cursor);
  rb_scan_args(argc, argv, "11", &key, &direction);
  if (NIL_P(direction))
    direction = INT2NUM(0);
  rc = unqlite_kv_cursor_seek(cursor, RSTRING_PTR(key), RSTRING_LEN(key), NUM2INT(direction));
  CHECK(0, rc);
  return Qtrue;
}

/*
 * call-seq:
 *    cursor.key
 *
 * Returns the key of the entry pointed to by the cursor.
 */
static VALUE unqlite_cursor_key(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  int rc;
  volatile VALUE rkey;
  int key_size;

  GetCursor2(self, rcursor, cursor);
  rc = unqlite_kv_cursor_key(cursor, NULL, &key_size);
  CHECK(0, rc);
  rkey = rb_str_buf_new(key_size);
  rc = unqlite_kv_cursor_key(cursor, RSTRING_PTR(rkey), &key_size);
  CHECK(0, rc);
  rb_str_set_len(rkey, key_size);
  return rkey;
}

/*
 * call-seq:
 *    cursor.value
 *    cursor.data
 *
 * Returns the data/value of the entry pointed to by the cursor.
 */
static VALUE unqlite_cursor_value(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  int rc;
  volatile VALUE rvalue;
  unqlite_int64 data_size;

  GetCursor2(self, rcursor, cursor);
  rc = unqlite_kv_cursor_data(cursor, NULL, &data_size);
  CHECK(0, rc);
  rvalue = rb_str_buf_new(data_size);
  rc = unqlite_kv_cursor_data(cursor, RSTRING_PTR(rvalue), &data_size);
  CHECK(0, rc);
  rb_str_set_len(rvalue, data_size);
  return rvalue;
}

/*
 * call-seq:
 *   cursor.release
 *   cursor.close
 *
 * Releases the underlying cursor.
 */
VALUE unqlite_cursor_release(VALUE self)
{
  unqliteRubyCursor* rcursor;
  unqlite_kv_cursor* cursor;
  int rc;

  GetCursor2(self, rcursor, cursor);

  if (rcursor->cursor) {
    unqliteRubyPtr rdatabase;
    Data_Get_Struct(rcursor->rb_database, unqliteRuby, rdatabase);
    rc = unqlite_kv_cursor_release(rdatabase->pDb, rcursor->cursor);
    CHECK(0, rc);
    rcursor->cursor = NULL;
    rcursor->rb_database = Qnil;
    rb_ary_delete(rdatabase->acursors, self);
  }

  return Qtrue;
}

void Init_unqlite_cursor()
{
  VALUE mUnQLite = rb_path2class("UnQLite");
  /* Cursors provide a mechanism by which you can iterate over the records in a database. Using cursors, you can seek, fetch, move, and delete database records. */
  VALUE cUnQLiteCursor = rb_define_class_under(mUnQLite, "Cursor", rb_cObject);
  rb_define_alloc_func(cUnQLiteCursor, unqlite_cursor_allocate);
  rb_define_method(cUnQLiteCursor, "initialize", unqlite_cursor_initialize, 1);
  rb_define_method(cUnQLiteCursor, "release", unqlite_cursor_release, 0);
  rb_define_method(cUnQLiteCursor, "close", unqlite_cursor_release, 0);
  rb_define_method(cUnQLiteCursor, "reset", unqlite_cursor_reset, 0);
  rb_define_method(cUnQLiteCursor, "seek", unqlite_cursor_seek, -1);
  rb_define_method(cUnQLiteCursor, "first!", unqlite_cursor_first, 0);
  rb_define_method(cUnQLiteCursor, "last!", unqlite_cursor_last, 0);
  rb_define_method(cUnQLiteCursor, "valid?", unqlite_cursor_valid, 0);
  rb_define_method(cUnQLiteCursor, "next!", unqlite_cursor_next, 0);
  rb_define_method(cUnQLiteCursor, "prev!", unqlite_cursor_prev, 0);
  rb_define_method(cUnQLiteCursor, "key", unqlite_cursor_key, 0);
  rb_define_method(cUnQLiteCursor, "value", unqlite_cursor_value, 0);
  rb_define_method(cUnQLiteCursor, "data", unqlite_cursor_value, 0);
  rb_define_method(cUnQLiteCursor, "delete!", unqlite_cursor_delete, 0);
}
