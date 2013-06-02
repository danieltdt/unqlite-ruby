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

  // Get unqlite stored inside ruby structure
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

static VALUE unqlite_database_store(VALUE self)
{
  return rb_str_new2("TODO: implement #store");
}

static VALUE unqlite_database_fetch(VALUE self)
{
  return rb_str_new2("TODO: implement #fetch");
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
