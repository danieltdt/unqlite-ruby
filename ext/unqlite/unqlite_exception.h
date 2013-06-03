#ifndef UNQLITE_RUBY_EXCEPTION
#define UNQLITE_RUBY_EXCEPTION

#include <unqlite_ruby.h>

/* Macro to raise the proper exception given a return code */
#define CHECK(_db, _rc) rb_unqlite_raise(_db, _rc);

#endif
