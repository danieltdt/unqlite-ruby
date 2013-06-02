#ifndef UNQLITE_RUBY_DATABASE
#define UNQLITE_RUBY_DATABASE

#include <unqlite_ruby.h>

struct _unqliteRuby {
  unqlite *pDb;
};

typedef struct _unqliteRuby unqliteRuby;
typedef unqliteRuby * unqliteRubyPtr;

extern VALUE cUnQLiteDatabase;

void Init_unqlite_database();

#endif
