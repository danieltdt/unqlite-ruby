#include <unqlite_database.h>

VALUE cUnQLiteDatabase;

static void deallocate(void *ctx)
{
  unqliteRubyPtr c = (unqliteRubyPtr) ctx;
  unqlite *pDb = c->pDb;

  if (pDb) unqlite_close(pDb);
  xfree(c);
}

static VALUE allocate(VALUE klass)
{
  unqliteRubyPtr ctx = malloc(sizeof(unqliteRuby));
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
  c_string = calloc(RSTRING_LEN(rb_string), sizeof(char));
  memcpy(c_string, StringValuePtr(rb_string), RSTRING_LEN(rb_string));

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

  // Close database
  rc = unqlite_close(ctx->pDb);

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;
}


static VALUE unqlite_database_store(VALUE self, VALUE key, VALUE value)
{
  void *c_key;
  void *c_value;
  int rc;
  unqliteRubyPtr ctx;

  // Ensure the given argument is a ruby string
  Check_Type(key, T_STRING);
  Check_Type(value, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Transform Ruby string into C string
  c_key = calloc(RSTRING_LEN(key), sizeof(char));
  memcpy(c_key, StringValuePtr(key), RSTRING_LEN(key));

  c_value = calloc(RSTRING_LEN(value), sizeof(char));
  memcpy(c_value, StringValuePtr(value), RSTRING_LEN(value));

  // Store it
  rc = unqlite_kv_store(ctx->pDb, c_key, -1, c_value, sizeof(c_value));

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;
}

static VALUE unqlite_database_delete(VALUE self, VALUE key)
{
  void *c_key;
  int rc;
  unqliteRubyPtr ctx;

  // Ensure the given argument is a ruby string
  Check_Type(key, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Transform Ruby string into C string
  c_key = calloc(RSTRING_LEN(key), sizeof(char));
  memcpy(c_key, StringValuePtr(key), RSTRING_LEN(key));

  // Delete it
  rc = unqlite_kv_delete(ctx->pDb, c_key, -1);

  // Check for errors
  CHECK(ctx->pDb, rc);

  return Qtrue;

}

static VALUE unqlite_database_fetch(VALUE self, VALUE collection_name)
{
  void *c_collection_name;
  void *fetched_data;
  unqlite_int64 n_bytes;
  int rc;
  unqliteRubyPtr ctx;

  // Ensure the given argument is a ruby string
  Check_Type(collection_name, T_STRING);

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  // Transform Ruby string into C string
  c_collection_name = calloc(RSTRING_LEN(collection_name), sizeof(char));
  memcpy(c_collection_name, StringValuePtr(collection_name), RSTRING_LEN(collection_name));

  // Extract the data size, check for errors and return if any
  rc = unqlite_kv_fetch(ctx->pDb, c_collection_name, -1, NULL, &n_bytes);
  CHECK(ctx->pDb, rc);
  if( rc != UNQLITE_OK ) { return Qnil; }

  // Data is empty
  fetched_data = (char *)malloc(n_bytes);
  if( fetched_data == NULL ) { return rb_str_new2(""); }

  // Now, fetch the data
  rc = unqlite_kv_fetch(ctx->pDb, c_collection_name, -1, fetched_data, &n_bytes);
  CHECK(ctx->pDb, rc);

  return rb_str_new2((char *)fetched_data);
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
