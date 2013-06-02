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
  // TODO: Always opening as a memory database. It should be a parameter.
  rc = unqlite_open(&ctx->pDb, c_string, UNQLITE_OPEN_IN_MEMORY);

  // Verify if is everything ok
  if( rc != UNQLITE_OK ){
    // TODO: Return error code with a better message raising a UnQLite exception.
    rb_raise(rb_eRuntimeError, "Couldn't open database");

    return self;
  }

  return self;
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

  rc = unqlite_kv_store(ctx->pDb, c_key, -1, c_value, sizeof(c_value));

  if( rc != UNQLITE_OK ) {
    if( rc != UNQLITE_BUSY && rc != UNQLITE_NOTIMPLEMENTED ) {
      unqlite_rollback(ctx->pDb);
    }
  }

  return INT2FIX(rc);
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

  rc = unqlite_kv_fetch(ctx->pDb, c_collection_name, -1, NULL, &n_bytes);
  if( rc != UNQLITE_OK ) { return INT2FIX(rc); }

  fetched_data = (char *)malloc(n_bytes);
  if( fetched_data == NULL ) { return INT2FIX(UNQLITE_EMPTY); }

  rc = unqlite_kv_fetch(ctx->pDb, c_collection_name, -1, fetched_data, &n_bytes);
  if( rc == UNQLITE_OK ) { return rb_str_new2((char *)fetched_data); }

  return INT2FIX(rc);
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

  rb_define_method(cUnQLiteDatabase, "initialize", initialize, 1);
}
