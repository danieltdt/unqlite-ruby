#include <unqlite_ruby.h>

VALUE mUnQLite;

void Init_unqlite_native()
{
  mUnQLite = rb_define_module("UnQLite");

  Init_unqlite_database();
  Init_unqlite_codes();
}
