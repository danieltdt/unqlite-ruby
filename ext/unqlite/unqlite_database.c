#include <unqlite_database.h>

VALUE cUnQLiteDatabase;

static void deallocate(void *ctx)
{
  unqliteRubyPtr c = (unqliteRubyPtr) ctx;
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

static VALUE initialize(VALUE self, VALUE rb_string)
{
  void *c_string;
  int rc;
  unqliteRubyPtr ctx;

  // Ensure the given argument is a ruby string
  Check_Type(rb_string, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Transform Ruby string into C string
  c_string = StringValueCStr(rb_string);

  // Open database
  // TODO: Accept others open mode (read-only + mmap, etc. Check http://unqlite.org/c_api/unqlite_open.html)
  rc = unqlite_open(&ctx->pDb, c_string, UNQLITE_OPEN_CREATE);

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
  VALUE rb_string;

  // Ensure the given argument is a ruby string
  Check_Type(collection_name, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Extract the data size, check for errors and return if any
  rc = unqlite_kv_fetch(ctx->pDb, StringValuePtr(collection_name), RSTRING_LEN(collection_name), NULL, &n_bytes);

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

void Init_unqlite_database()
{
#if 0
  VALUE mUnqlite = rb_define_module("UnQLite");
#endif

  /* defining UnQLite::Database class and appending its methods */
  cUnQLiteDatabase = rb_define_class_under(mUnQLite, "Database", rb_cObject);

  rb_define_alloc_func(cUnQLiteDatabase, allocate);

  rb_define_method(cUnQLiteDatabase, "store", unqlite_database_store, 2);
  rb_define_method(cUnQLiteDatabase, "fetch", unqlite_database_fetch, 1);
  rb_define_method(cUnQLiteDatabase, "delete", unqlite_database_delete, 1);

  rb_define_method(cUnQLiteDatabase, "begin_transaction", unqlite_database_begin_transaction, 0);
  rb_define_method(cUnQLiteDatabase, "commit", unqlite_database_commit, 0);
  rb_define_method(cUnQLiteDatabase, "rollback", unqlite_database_rollback, 0);

  rb_define_method(cUnQLiteDatabase, "initialize", initialize, 1);
  rb_define_method(cUnQLiteDatabase, "close", unqlite_database_close, 0);
}
