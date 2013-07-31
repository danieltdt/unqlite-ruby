#include <unqlite_database.h>

VALUE cUnQLiteDatabase;

static void deallocate(unqliteRubyPtr c)
{
  unqlite *pDb = c->pDb;
  c->pDb = 0;
  if (pDb) unqlite_close(pDb);
  xfree(c);
}

static VALUE allocate(VALUE klass)
{
  unqliteRubyPtr ctx = ALLOC(unqliteRuby);
  ctx->pDb = NULL;

  return Data_Wrap_Struct(klass, NULL, deallocate, ctx);
}

/*
 * call-seq:
 *     UnQLite::Database.new(filename, flags = nil)
 *     UnQLite::Database.new(filename, flags = nil) { |unqlite| ... }
 *
 * Creates a new UnQLite instance by opening an unqlite file named _filename_.
 * If the file does not exist, a new file will be
 * created. _flags_ may be one of the following:
 *
 * * *UnQlite::CREATE*  - Create if database does not exist.
 * * *UnQLite::READWRITE*  - Open the database with READ+WRITE priviledged.
 * * *UnQLite::READONLY* - Open the database in read-only mode.
 * * *UnQLite::MMAP*  - Obtain a read-only memory view of the whole database.
 * * *UnQLite::TEMP_DB*  - A private, temporary on-disk database will be created.
 * * *UnQLite::IN_MEMORY*  - A private, on-memory database will be created.
 * * *UnQLite::OMIT_JOURNALING*  - Disable journaling for this database.
 * * *UnQLite::NOMUTEX*  - Disable the private recursive mutex associated with each database handle.
 *
 * If no _flags_ are specified, the UnQLite object will try to open the database
 * file as a writer and will create it if it does not already exist
 * (cf. flag <tt>CREATE</tt>).
 */
static VALUE initialize(int argc, VALUE* argv, VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  VALUE filename, vflags;
  int flags = UNQLITE_OPEN_CREATE;

  rb_scan_args(argc, argv, "11", &filename, &vflags);

  // Get the flags if specified
  if (!NIL_P(vflags))
    flags = NUM2INT(vflags);

  // Ensure the given argument is a ruby string
  Check_Type(filename, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Open database
  rc = unqlite_open(&ctx->pDb, StringValueCStr(filename), flags);

  // Check if any exception should be raised
  CHECK(ctx->pDb, rc);

  return self;
}

static VALUE unqlite_database_close(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  if (ctx->pDb)
  {
    // Close database
    rc = unqlite_close(ctx->pDb);

    // Check for errors
    CHECK(ctx->pDb, rc);
  }

  ctx->pDb = 0;

  return Qtrue;
}

static VALUE unqlite_database_closed(VALUE self)
{
  unqliteRubyPtr ctx;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  if (ctx->pDb)
  {
     return Qfalse;
  }
  else
  {
     return Qtrue;
  }
}

/*
 * call-seq:
 *     UnQLite::Database.open(filename, flags = nil)
 *
 * If called without a block, this is synonymous to
 * UnQLite::Database::new.  If a block is given, the new UnQLite
 * instance will be passed to the block as a parameter, and the
 * corresponding database file will be closed after the execution of
 * the block code has been finished.
 *
 * Example for an open call with a block:
 *
 *   require 'unqlite'
 *   UnQLite::Database.open("fruitstore.db") do |unq|
 *     unq.each_pair do |key, value|
 *       print "#{key}: #{value}\n"
 *     end
 *   end
 */
static VALUE unqlite_database_open(int argc, VALUE* argv, VALUE klass)
{
  VALUE obj = allocate(klass);

  if (NIL_P(initialize(argc, argv, obj)))
    return Qnil;

  if (rb_block_given_p())
    return rb_ensure(rb_yield, obj, unqlite_database_close, obj);
  else
    return obj;
}

static VALUE unqlite_database_store(VALUE self, VALUE key, VALUE value)
{
  int rc;
  unqliteRubyPtr ctx;

  // Ensure the given argument is a ruby string
  Check_Type(key, T_STRING);
  Check_Type(value, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Store it
  rc = unqlite_kv_store(ctx->pDb, StringValuePtr(key), RSTRING_LEN(key), StringValuePtr(value), RSTRING_LEN(value));

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_append(VALUE self, VALUE key, VALUE value)
{
  int rc;
  unqliteRubyPtr ctx;

  // Ensure the given argument is a ruby string
  Check_Type(key, T_STRING);
  Check_Type(value, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Append it
  rc = unqlite_kv_append(ctx->pDb, StringValuePtr(key), RSTRING_LEN(key), StringValuePtr(value), RSTRING_LEN(value));

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_delete(VALUE self, VALUE key)
{
  int rc;
  unqliteRubyPtr ctx;

  // Ensure the given argument is a ruby string
  Check_Type(key, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Delete it
  rc = unqlite_kv_delete(ctx->pDb, StringValuePtr(key), RSTRING_LEN(key));

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_fetch(VALUE self, VALUE collection_name)
{
  unqlite_int64 n_bytes;
  int rc;
  unqliteRubyPtr ctx;
  VALUE filename;

  // Ensure the given argument is a ruby string
  Check_Type(collection_name, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Extract the data size, check for errors and return if any
  rc = unqlite_kv_fetch(ctx->pDb, StringValuePtr(collection_name), RSTRING_LEN(collection_name), NULL, &n_bytes);

  CHECK(ctx->pDb, rc);
  if( rc != UNQLITE_OK ) { return Qnil; }

  // Allocate string buffer object
  filename = rb_str_buf_new(n_bytes);

  // Now, fetch the data
  rc = unqlite_kv_fetch(ctx->pDb, StringValuePtr(collection_name), RSTRING_LEN(collection_name), RSTRING_PTR(filename), &n_bytes);
  CHECK(ctx->pDb, rc);

  rb_str_set_len(filename, n_bytes);

  return filename;
}

static VALUE unqlite_database_has_key(VALUE self, VALUE collection_name)
{
  unqliteRubyPtr ctx;
  int rc;
  unqlite_int64 n_bytes;

  // Ensure the given argument is a ruby string
  Check_Type(collection_name, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Extract the data size, check for errors and return if any
  rc = unqlite_kv_fetch(ctx->pDb, StringValuePtr(collection_name), RSTRING_LEN(collection_name), NULL, &n_bytes);
  if (rc == UNQLITE_NOTFOUND)
  {
     return Qfalse;
  }
  else
  {
     CHECK(ctx->pDb, rc);
     return Qtrue;
  }
}

static VALUE unqlite_database_aref(VALUE self, VALUE collection_name)
{
  unqlite_int64 n_bytes;
  int rc;
  unqliteRubyPtr ctx;
  VALUE rb_string;

  // Ensure the given argument is a ruby string
  Check_Type(collection_name, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Extract the data size, check for errors and return if any
  rc = unqlite_kv_fetch(ctx->pDb, StringValuePtr(collection_name), RSTRING_LEN(collection_name), NULL, &n_bytes);

  if (rc == UNQLITE_NOTFOUND)
     return Qnil;

  CHECK(ctx->pDb, rc);
  if( rc != UNQLITE_OK ) { return Qnil; }

  // Allocate string buffer object
  rb_string = rb_str_buf_new(n_bytes);

  // Now, fetch the data
  rc = unqlite_kv_fetch(ctx->pDb, StringValuePtr(collection_name), RSTRING_LEN(collection_name), RSTRING_PTR(rb_string), &n_bytes);
  CHECK(ctx->pDb, rc);

  rb_str_set_len(rb_string, n_bytes);

  return rb_string;
}

static VALUE unqlite_database_begin_transaction(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Begin write-transaction manually
  rc = unqlite_begin(ctx->pDb);

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_commit(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Commit transaction
  rc = unqlite_commit(ctx->pDb);

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_rollback(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Rollback transaction
  rc = unqlite_rollback(ctx->pDb);

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_end_transaction(VALUE self, VALUE commit)
{
  if (RTEST(commit))
    return unqlite_database_commit(self);
  else
    return unqlite_database_rollback(self);
}

static VALUE unqlite_database_transaction_rescue(VALUE self, VALUE error)
{
  unqlite_database_rollback(self);
  rb_exc_raise(error);
}

static VALUE unqlite_database_transaction_body(VALUE vdb)
{
  return rb_rescue(rb_yield, vdb, unqlite_database_transaction_rescue, vdb);
}

static VALUE unqlite_database_transaction_ensure(VALUE self, VALUE vbargs)
{
  return unqlite_database_commit(self);
}

static VALUE unqlite_database_transaction(VALUE self)
{
  VALUE vrv = unqlite_database_begin_transaction(self);
  if (vrv != Qtrue)
    return Qfalse;
  rb_ensure(unqlite_database_transaction_body, self, unqlite_database_transaction_ensure, self);
  return Qtrue;
}

static VALUE unqlite_database_each(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite_kv_cursor *cursor;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  rc = unqlite_kv_cursor_init(ctx->pDb, &cursor);
  CHECK(ctx->pDb, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  while (unqlite_kv_cursor_valid_entry(cursor))
  {
     int key_size;
     unqlite_int64 data_size;
     VALUE rb_key, rb_data;

     // Create Ruby String with key
     rc = unqlite_kv_cursor_key(cursor, NULL, &key_size);
     CHECK(ctx->pDb, rc);
     rb_key = rb_str_buf_new(key_size);
     rc = unqlite_kv_cursor_key(cursor, RSTRING_PTR(rb_key), &key_size);
     CHECK(ctx->pDb, rc);
     rb_str_set_len(rb_key, key_size);

     // Create Ruby String with data
     rc = unqlite_kv_cursor_data(cursor, NULL, &data_size);
     CHECK(ctx->pDb, rc);
     rb_data = rb_str_buf_new(data_size);
     rc = unqlite_kv_cursor_data(cursor, RSTRING_PTR(rb_data), &data_size);
     CHECK(ctx->pDb, rc);
     rb_str_set_len(rb_data, data_size);

     // Yield to block
     rb_yield_values(2, rb_key, rb_data);

     rc = unqlite_kv_cursor_next_entry(cursor);
  }

  rc = unqlite_kv_cursor_release(ctx->pDb, cursor);
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_each_value(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite_kv_cursor *cursor;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  rc = unqlite_kv_cursor_init(ctx->pDb, &cursor);
  CHECK(ctx->pDb, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  while (unqlite_kv_cursor_valid_entry(cursor))
  {
     unqlite_int64 data_size;
     VALUE rb_data;

     // Create Ruby String with data
     rc = unqlite_kv_cursor_data(cursor, NULL, &data_size);
     CHECK(ctx->pDb, rc);
     rb_data = rb_str_buf_new(data_size);
     rc = unqlite_kv_cursor_data(cursor, RSTRING_PTR(rb_data), &data_size);
     CHECK(ctx->pDb, rc);
     rb_str_set_len(rb_data, data_size);

     // Yield to block
     rb_yield_values(1, rb_data);

     rc = unqlite_kv_cursor_next_entry(cursor);
  }

  rc = unqlite_kv_cursor_release(ctx->pDb, cursor);
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_each_key(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite_kv_cursor *cursor;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  rc = unqlite_kv_cursor_init(ctx->pDb, &cursor);
  CHECK(ctx->pDb, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  while (unqlite_kv_cursor_valid_entry(cursor))
  {
     int key_size;
     VALUE rb_key;

     // Create Ruby String with key
     rc = unqlite_kv_cursor_key(cursor, NULL, &key_size);
     CHECK(ctx->pDb, rc);
     rb_key = rb_str_buf_new(key_size);
     rc = unqlite_kv_cursor_key(cursor, RSTRING_PTR(rb_key), &key_size);
     CHECK(ctx->pDb, rc);
     rb_str_set_len(rb_key, key_size);

     // Yield to block
     rb_yield_values(1, rb_key);

     rc = unqlite_kv_cursor_next_entry(cursor);
  }

  rc = unqlite_kv_cursor_release(ctx->pDb, cursor);
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_clear(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite_kv_cursor *cursor;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  rc = unqlite_kv_cursor_init(ctx->pDb, &cursor);
  CHECK(ctx->pDb, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  while (unqlite_kv_cursor_valid_entry(cursor))
  {
     rc = unqlite_kv_cursor_delete_entry(cursor);
     CHECK(ctx->pDb, rc);

     rc = unqlite_kv_cursor_first_entry(cursor);
  }

  rc = unqlite_kv_cursor_release(ctx->pDb, cursor);
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_empty(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite_kv_cursor *cursor;
  VALUE result;

  Data_Get_Struct(self, unqliteRuby, ctx);

  rc = unqlite_kv_cursor_init(ctx->pDb, &cursor);
  CHECK(ctx->pDb, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  result = unqlite_kv_cursor_valid_entry(cursor) ? Qfalse : Qtrue;

  rc = unqlite_kv_cursor_release(ctx->pDb, cursor);
  CHECK(ctx->pDb, rc);

  return result;
}

void Init_unqlite_database()
{
#if 0
  VALUE mUnqlite = rb_define_module("UnQLite");
#endif

  VALUE mUnqlite = rb_path2class("UnQLite");

  /* flag for #new and #open: If the database does not exists, it is created. Otherwise, it is opened with read+write privileges. */
  rb_define_const(mUnqlite, "CREATE", INT2FIX(UNQLITE_OPEN_CREATE));
  /* flag for #new and #open: Open the database with read+write privileges. */
  rb_define_const(mUnqlite, "READWRITE", INT2FIX(UNQLITE_OPEN_READWRITE));
  /* flag for #new and #open: Open the database in a read-only mode. */
  rb_define_const(mUnqlite, "READONLY", INT2FIX(UNQLITE_OPEN_READONLY));
  /* flag for #new and #open: Obtain a read-only memory view of the whole database. This flag works only in conjunction with the READONLY control flag */
  rb_define_const(mUnqlite, "MMAP", INT2FIX(UNQLITE_OPEN_MMAP));
  /* flag for #new and #open: A private, temporary on-disk database will be created. This private database will be automatically deleted as soon as the database connection is closed. */
  rb_define_const(mUnqlite, "TEMP_DB", INT2FIX(UNQLITE_OPEN_TEMP_DB));
  /* flag for #new and #open: A private, temporary on-disk database will be created. This private database will be automatically deleted as soon as the database connection is closed. */
  rb_define_const(mUnqlite, "IN_MEMORY", INT2FIX(UNQLITE_OPEN_IN_MEMORY));
  /* flag for #new and #open: Disable journaling for this database. */
  rb_define_const(mUnqlite, "OMIT_JOURNALING", INT2FIX(UNQLITE_OPEN_OMIT_JOURNALING));
  /* flag for #new and #open: Disable the private recursive mutex associated with each database handle. */
  rb_define_const(mUnqlite, "NOMUTEX", INT2FIX(UNQLITE_OPEN_NOMUTEX));

  /* defining UnQLite::Database class and appending its methods */
  cUnQLiteDatabase = rb_define_class_under(mUnQLite, "Database", rb_cObject);

  rb_define_alloc_func(cUnQLiteDatabase, allocate);

  rb_define_singleton_method(cUnQLiteDatabase, "open", unqlite_database_open, -1);

  rb_define_method(cUnQLiteDatabase, "store", unqlite_database_store, 2);
  rb_define_method(cUnQLiteDatabase, "append", unqlite_database_append, 2);
  rb_define_method(cUnQLiteDatabase, "fetch", unqlite_database_fetch, 1);
  rb_define_method(cUnQLiteDatabase, "delete", unqlite_database_delete, 1);

  rb_define_method(cUnQLiteDatabase, "closed?", unqlite_database_closed, 0);
  rb_define_method(cUnQLiteDatabase, "[]", unqlite_database_aref, 1);
  rb_define_method(cUnQLiteDatabase, "[]=", unqlite_database_store, 2);
  rb_define_method(cUnQLiteDatabase, "has_key?", unqlite_database_has_key, 1);
  rb_define_method(cUnQLiteDatabase, "include?", unqlite_database_has_key, 1);
  rb_define_method(cUnQLiteDatabase, "key?", unqlite_database_has_key, 1);
  rb_define_method(cUnQLiteDatabase, "member?", unqlite_database_has_key, 1);
  rb_define_method(cUnQLiteDatabase, "clear", unqlite_database_clear, 0);
  rb_define_method(cUnQLiteDatabase, "empty?", unqlite_database_empty, 0);

  rb_define_method(cUnQLiteDatabase, "each", unqlite_database_each, 0);
  rb_define_method(cUnQLiteDatabase, "each_pair", unqlite_database_each, 0);
  rb_define_method(cUnQLiteDatabase, "each_key", unqlite_database_each_key, 0);
  rb_define_method(cUnQLiteDatabase, "each_value", unqlite_database_each_value, 0);

  rb_define_method(cUnQLiteDatabase, "begin_transaction", unqlite_database_begin_transaction, 0);
  rb_define_method(cUnQLiteDatabase, "end_transaction", unqlite_database_end_transaction, 1);
  rb_define_method(cUnQLiteDatabase, "commit", unqlite_database_commit, 0);
  rb_define_method(cUnQLiteDatabase, "rollback", unqlite_database_rollback, 0);
  rb_define_method(cUnQLiteDatabase, "transaction", unqlite_database_transaction, 0);

  rb_define_method(cUnQLiteDatabase, "initialize", initialize, -1);
  rb_define_method(cUnQLiteDatabase, "close", unqlite_database_close, 0);
}
