#include <unqlite_database.h>
#include <unqlite_cursor.h>

VALUE cUnQLiteDatabase;

/* Get database context pointer from Ruby object */
#define GetDatabase(obj, databasep) {                 \
    Data_Get_Struct((obj), unqliteRuby, (databasep)); \
    if ((databasep) == 0) closed_database();          \
    if ((databasep)->pDb == 0) closed_database();     \
  }

/* Get database context pointer and native unqlite pointer from Ruby object */
#define GetDatabase2(obj, databasep, database) { \
    GetDatabase((obj), (databasep));             \
    (database) = (databasep)->pDb;               \
  }

/* Raise error for already closed database */
static void closed_database()
{
  rb_raise(rb_eRuntimeError, "Closed database");
}

static void unqliteRuby_close(unqliteRubyPtr ctx)
{
  if (ctx->pDb)
  {
    VALUE cur;
    int rc;

    /* close lingering cursors */
    if (!NIL_P(ctx->acursors) && RARRAY_LEN(ctx->acursors) > 0) {
      while ((cur = rb_ary_pop(ctx->acursors)) != Qnil)
        unqlite_cursor_release(cur);
    }

    // Close database
    rc = unqlite_close(ctx->pDb);

    // Check for errors
    CHECK(ctx->pDb, rc);
  }

  ctx->pDb = 0;
}

/* Wrapped object: mark */
static void unqlite_database_mark(unqliteRubyPtr rdatabase)
{
  rb_gc_mark(rdatabase->acursors);
}

/* Wrapped object: deallocate */
static void unqlite_database_deallocate(unqliteRubyPtr c)
{
  unqliteRuby_close(c);
  xfree(c);
}

/* Wrapped object: allocate */
static VALUE unqlite_database_allocate(VALUE klass)
{
  unqliteRubyPtr ctx = ALLOC(unqliteRuby);
  volatile VALUE rb_database;
  ctx->pDb = NULL;
  ctx->acursors = Qnil;
  rb_database = Data_Wrap_Struct(klass, unqlite_database_mark, unqlite_database_deallocate, ctx);
  return rb_database;
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
 * * +UnQlite::CREATE+  - Create if database does not exist.
 * * +UnQLite::READWRITE+  - Open the database with READ+WRITE priviledged.
 * * +UnQLite::READONLY+ - Open the database in read-only mode.
 * * +UnQLite::MMAP+  - Obtain a read-only memory view of the whole database.
 * * +UnQLite::TEMP_DB+  - A private, temporary on-disk database will be created.
 * * +UnQLite::IN_MEMORY+  - A private, on-memory database will be created.
 * * +UnQLite::OMIT_JOURNALING+  - Disable journaling for this database.
 * * +UnQLite::NOMUTEX+  - Disable the private recursive mutex associated with each database handle.
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

  Data_Get_Struct(self, unqliteRuby, ctx);

  // Open database
  rc = unqlite_open(&ctx->pDb, StringValueCStr(filename), flags);

  ctx->acursors = rb_ary_new();

  // Check if any exception should be raised
  CHECK(ctx->pDb, rc);

  return self;
}

/*
 * call-seq:
 *    database.close
 *
 * Closes the database (automatically commits any open transaction).
 */
static VALUE unqlite_database_close(VALUE self)
{
  unqliteRubyPtr ctx;

  // Get class context
  Data_Get_Struct(self, unqliteRuby, ctx);

  unqliteRuby_close(ctx);
  return Qtrue;
}

/*
 * call-seq:
 *    database.closed?  -> true or false
 *
 * Returns true if the associated database has been closed.
 */
static VALUE unqlite_database_closed(VALUE self)
{
  unqliteRubyPtr ctx;
  unqlite* db;

  Data_Get_Struct(self, unqliteRuby, ctx);
  db = ctx->pDb;

  if (db)
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
  volatile VALUE obj = unqlite_database_allocate(klass);

  if (NIL_P(initialize(argc, argv, obj)))
    return Qnil;

  if (rb_block_given_p())
    return rb_ensure(rb_yield, obj, unqlite_database_close, obj);
  else
    return obj;
}


/*
 * call-seq:
 *    database.store key, value
 *    database[key] = value
 *
 * Associates the value _value_ with the specified _key_.
 */
static VALUE unqlite_database_store(VALUE self, VALUE key, VALUE value)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  // Ensure the given argument is a ruby string
  Check_Type(key, T_STRING);
  Check_Type(value, T_STRING);

  GetDatabase2(self, ctx, db);

  // Store it
  rc = unqlite_kv_store(db, StringValuePtr(key), RSTRING_LEN(key), StringValuePtr(value), RSTRING_LEN(value));

  // Check for errors
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *     database.append key, value
 *
 * Appends the _value_ to an already existing value associated with _key_.
 */
static VALUE unqlite_database_append(VALUE self, VALUE key, VALUE value)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  // Ensure the given argument is a ruby string
  Check_Type(key, T_STRING);
  Check_Type(value, T_STRING);

  GetDatabase2(self, ctx, db);

  // Append it
  rc = unqlite_kv_append(db, StringValuePtr(key), RSTRING_LEN(key), StringValuePtr(value), RSTRING_LEN(value));

  // Check for errors
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *     database.delete key
 *
 * Removes the key-value pair with the specified _key_ from this database.
 */
static VALUE unqlite_database_delete(VALUE self, VALUE key)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  // Ensure the given argument is a ruby string
  Check_Type(key, T_STRING);

  GetDatabase2(self, ctx, db);

  // Delete it
  rc = unqlite_kv_delete(db, StringValuePtr(key), RSTRING_LEN(key));

  // Check for errors
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *    database.fetch(key) -> value
 *
 * Retrieves the _value_ corresponding to _key_. If there is no value
 * associated with _key_, an exception will be raised.
 */
static VALUE unqlite_database_fetch(VALUE self, VALUE collection_name)
{
  unqlite_int64 n_bytes;
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;
  volatile VALUE filename;

  // Ensure the given argument is a ruby string
  Check_Type(collection_name, T_STRING);

  GetDatabase2(self, ctx, db);

  // Extract the data size, check for errors and return if any
  rc = unqlite_kv_fetch(db, StringValuePtr(collection_name), RSTRING_LEN(collection_name), NULL, &n_bytes);

  CHECK(db, rc);
  if( rc != UNQLITE_OK ) { return Qnil; }

  // Allocate string buffer object
  filename = rb_str_buf_new(n_bytes);

  // Now, fetch the data
  rc = unqlite_kv_fetch(db, StringValuePtr(collection_name), RSTRING_LEN(collection_name), RSTRING_PTR(filename), &n_bytes);
  CHECK(db, rc);

  rb_str_set_len(filename, n_bytes);

  return filename;
}

/*
 * call-seq:
 *     database.has_key?(key)
 *     database.key?(key)
 *     database.include?(key)
 *     database.member?(key)
 *
 * Returns true if the given _key_ exists within the database. Returns
 * false otherwise.
 */
static VALUE unqlite_database_has_key(VALUE self, VALUE collection_name)
{
  unqliteRubyPtr ctx;
  unqlite* db;
  int rc;
  unqlite_int64 n_bytes;

  // Ensure the given argument is a ruby string
  Check_Type(collection_name, T_STRING);

  GetDatabase2(self, ctx, db);

  // Extract the data size, check for errors and return if any
  rc = unqlite_kv_fetch(db, StringValuePtr(collection_name), RSTRING_LEN(collection_name), NULL, &n_bytes);
  if (rc == UNQLITE_NOTFOUND)
  {
     return Qfalse;
  }
  else
  {
     CHECK(db, rc);
     return Qtrue;
  }
}

/*
 * call-seq:
 *    database[key] -> value
 *
 * Retrieves the _value_ corresponding to _key_. If the key does not
 * exist in the database, nil is returned.
 */
static VALUE unqlite_database_aref(VALUE self, VALUE collection_name)
{
  unqlite_int64 n_bytes;
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;
  volatile VALUE rb_string;

  // Ensure the given argument is a ruby string
  Check_Type(collection_name, T_STRING);

  GetDatabase2(self, ctx, db);

  // Extract the data size, check for errors and return if any
  rc = unqlite_kv_fetch(db, StringValuePtr(collection_name), RSTRING_LEN(collection_name), NULL, &n_bytes);

  if (rc == UNQLITE_NOTFOUND)
     return Qnil;

  CHECK(db, rc);
  if( rc != UNQLITE_OK ) { return Qnil; }

  // Allocate string buffer object
  rb_string = rb_str_buf_new(n_bytes);

  // Now, fetch the data
  rc = unqlite_kv_fetch(db, StringValuePtr(collection_name), RSTRING_LEN(collection_name), RSTRING_PTR(rb_string), &n_bytes);
  CHECK(db, rc);

  rb_str_set_len(rb_string, n_bytes);

  return rb_string;
}

/*
 * call-seq:
 *     database.begin_transaction
 *
 * Begins a write-transaction. Ignored if a transaction has already
 * been opened.
 */
static VALUE unqlite_database_begin_transaction(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  GetDatabase2(self, ctx, db);

  // Begin write-transaction manually
  rc = unqlite_begin(db);

  // Check for errors
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *     database.commit
 *
 * Commit all changes to the database and release the exclusive lock.
 */
static VALUE unqlite_database_commit(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  GetDatabase2(self, ctx, db);

  // Commit transaction
  rc = unqlite_commit(db);

  // Check for errors
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *     database.rollback
 *
 * Rollback a write-transaction.
 */
static VALUE unqlite_database_rollback(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  GetDatabase2(self, ctx, db);

  // Rollback transaction
  rc = unqlite_rollback(db);

  // Check for errors
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *     database.end_transaction(true or false)
 *
 * Ends the current transaction. If argument is _true_, the
 * transaction is commited, if _false_ it is rolled back.
 */
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

/*
 * call-seq:
 *     database.transaction { |db| ... }
 *
 * Begins a write-transaction and executes _block_. If the block
 * executes to completion, the transaction is committed. If an
 * exception is raise by the block, the transaction is rolled back.
 */
static VALUE unqlite_database_transaction(VALUE self)
{
  VALUE vrv = unqlite_database_begin_transaction(self);
  if (vrv != Qtrue)
    return Qfalse;
  rb_ensure(unqlite_database_transaction_body, self, unqlite_database_transaction_ensure, self);
  return Qtrue;
}

/*
 * call-seq:
 *    database.each { |key, value|  ... }
 *    database.each_pair { |key, value|  ... }
 *
 * Executes _block_ for each key in the database, passing the _key_
 * and the corresponding _value_ as parameters.
 */
static VALUE unqlite_database_each(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;
  unqlite_kv_cursor *cursor;

  GetDatabase2(self, ctx, db);

  rc = unqlite_kv_cursor_init(db, &cursor);
  CHECK(db, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  while (unqlite_kv_cursor_valid_entry(cursor))
  {
     int key_size;
     unqlite_int64 data_size;
     volatile VALUE rb_key, rb_data;

     // Create Ruby String with key
     rc = unqlite_kv_cursor_key(cursor, NULL, &key_size);
     CHECK(db, rc);
     rb_key = rb_str_buf_new(key_size);
     rc = unqlite_kv_cursor_key(cursor, RSTRING_PTR(rb_key), &key_size);
     CHECK(db, rc);
     rb_str_set_len(rb_key, key_size);

     // Create Ruby String with data
     rc = unqlite_kv_cursor_data(cursor, NULL, &data_size);
     CHECK(db, rc);
     rb_data = rb_str_buf_new(data_size);
     rc = unqlite_kv_cursor_data(cursor, RSTRING_PTR(rb_data), &data_size);
     CHECK(db, rc);
     rb_str_set_len(rb_data, data_size);

     // Yield to block
     rb_yield_values(2, rb_key, rb_data);

     rc = unqlite_kv_cursor_next_entry(cursor);
  }

  rc = unqlite_kv_cursor_release(db, cursor);
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *    database.each_value { |value|  ... }
 *
 * Executes _block_ for each value in the database, passing the
 * _value_ as parameter.
 */
static VALUE unqlite_database_each_value(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;
  unqlite_kv_cursor *cursor;

  GetDatabase2(self, ctx, db);

  rc = unqlite_kv_cursor_init(db, &cursor);
  CHECK(db, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  while (unqlite_kv_cursor_valid_entry(cursor))
  {
     unqlite_int64 data_size;
     volatile VALUE rb_data;

     // Create Ruby String with data
     rc = unqlite_kv_cursor_data(cursor, NULL, &data_size);
     CHECK(db, rc);
     rb_data = rb_str_buf_new(data_size);
     rc = unqlite_kv_cursor_data(cursor, RSTRING_PTR(rb_data), &data_size);
     CHECK(db, rc);
     rb_str_set_len(rb_data, data_size);

     // Yield to block
     rb_yield_values(1, rb_data);

     rc = unqlite_kv_cursor_next_entry(cursor);
  }

  rc = unqlite_kv_cursor_release(db, cursor);
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *    database.each_key { |key|  ... }
 *
 * Executes _block_ for each key in the database, passing the _key_
 * and as parameter.
 */
static VALUE unqlite_database_each_key(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;
  unqlite_kv_cursor *cursor;

  GetDatabase2(self, ctx, db);

  rc = unqlite_kv_cursor_init(db, &cursor);
  CHECK(db, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  while (unqlite_kv_cursor_valid_entry(cursor))
  {
     int key_size;
     volatile VALUE rb_key;

     // Create Ruby String with key
     rc = unqlite_kv_cursor_key(cursor, NULL, &key_size);
     CHECK(db, rc);
     rb_key = rb_str_buf_new(key_size);
     rc = unqlite_kv_cursor_key(cursor, RSTRING_PTR(rb_key), &key_size);
     CHECK(db, rc);
     rb_str_set_len(rb_key, key_size);

     // Yield to block
     rb_yield_values(1, rb_key);

     rc = unqlite_kv_cursor_next_entry(cursor);
  }

  rc = unqlite_kv_cursor_release(db, cursor);
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *     database.clear
 *
 * Removes all the key-value pairs in the database.
 */
static VALUE unqlite_database_clear(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;
  unqlite_kv_cursor *cursor;

  GetDatabase2(self, ctx, db);

  rc = unqlite_kv_cursor_init(db, &cursor);
  CHECK(db, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  while (unqlite_kv_cursor_valid_entry(cursor))
  {
     rc = unqlite_kv_cursor_delete_entry(cursor);
     CHECK(db, rc);

     rc = unqlite_kv_cursor_first_entry(cursor);
  }

  rc = unqlite_kv_cursor_release(db, cursor);
  CHECK(db, rc);

  return Qtrue;
}

/*
 * call-seq:
 *     database.empty? -> true or false
 *
 * Returns _true_ if the database has no key-value pairs, false
 * otherwise.
 */
static VALUE unqlite_database_empty(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;
  unqlite_kv_cursor *cursor;
  volatile VALUE result;

  GetDatabase2(self, ctx, db);

  rc = unqlite_kv_cursor_init(db, &cursor);
  CHECK(db, rc);

  rc = unqlite_kv_cursor_first_entry(cursor);
  result = unqlite_kv_cursor_valid_entry(cursor) ? Qfalse : Qtrue;

  rc = unqlite_kv_cursor_release(db, cursor);
  CHECK(db, rc);

  return result;
}

/*
 * call-seq:
 *     database.max_page_cache = count
 *
 * Sets the maximum raw pages to cache in memory. This is a simple
 * hint, UnQLite is not forced to honor it.
 */
static VALUE unqlite_database_set_max_page_cache(VALUE self, VALUE count)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  GetDatabase2(self, ctx, db);

  rc = unqlite_config(db, UNQLITE_CONFIG_MAX_PAGE_CACHE, NUM2INT(count));
  CHECK(db, rc);

  return count;
}

/*
 * call-seq:
 *     database.kv_engine = engine
 *
 * Switch to another Key/Value storage engine.
 */
static VALUE unqlite_database_set_kv_engine(VALUE self, VALUE engine)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  GetDatabase2(self, ctx, db);

  SafeStringValue(engine);
  rc = unqlite_config(db, UNQLITE_CONFIG_KV_ENGINE, RSTRING_PTR(engine));
  CHECK(db, rc);

  return engine;
}

/*
 * call-seq:
 *     database.kv_engine -> engine
 *
 * Extract the name of the underlying Key/Value storage engine
 * (i.e. Hash, Mem, R+Tree, LSM, etc.).
 */
static VALUE unqlite_database_get_kv_engine(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;
  const char* name;

  GetDatabase2(self, ctx, db);

  rc = unqlite_config(db, UNQLITE_CONFIG_GET_KV_NAME, &name);
  CHECK(db, rc);

  return rb_str_new_cstr(name);
}

/*
 * call-seq:
 *     database.disable_auto_commit
 *
 * Normally, If #close is invoked while a transaction is open, the
 * transaction is automatically committed. But, if this option is set,
 * then the transaction is automatically rolled back and you should
 * call #commit manually to commit all database changes.
 */
static VALUE unqlite_database_disable_auto_commit(VALUE self)
{
  int rc;
  unqliteRubyPtr ctx;
  unqlite* db;

  GetDatabase2(self, ctx, db);

  rc = unqlite_config(db, UNQLITE_CONFIG_DISABLE_AUTO_COMMIT, 0);
  CHECK(db, rc);

  return Qnil;
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

  rb_define_alloc_func(cUnQLiteDatabase, unqlite_database_allocate);

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

  rb_define_method(cUnQLiteDatabase, "max_page_cache=", unqlite_database_set_max_page_cache, 1);
  rb_define_method(cUnQLiteDatabase, "kv_engine=", unqlite_database_set_kv_engine, 1);
  rb_define_method(cUnQLiteDatabase, "kv_engine", unqlite_database_get_kv_engine, 0);
  rb_define_method(cUnQLiteDatabase, "disable_auto_commit", unqlite_database_disable_auto_commit, 0);
}
