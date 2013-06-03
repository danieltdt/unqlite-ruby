#include <unqlite_codes.h>

VALUE mUnQLiteCodes;

void Init_unqlite_codes()
{
#if 0
  VALUE mUnqlite = rb_define_module("UnQLite");
#endif

  mUnQLiteCodes = rb_define_module_under(mUnQLite, "Codes");
  rb_define_const(mUnQLiteCodes, "OK",              INT2FIX(UNQLITE_OK));
  rb_define_const(mUnQLiteCodes, "NOMEM",           INT2FIX(UNQLITE_NOMEM));
  rb_define_const(mUnQLiteCodes, "ABORT",           INT2FIX(UNQLITE_ABORT));
  rb_define_const(mUnQLiteCodes, "IOERR",           INT2FIX(UNQLITE_IOERR));
  rb_define_const(mUnQLiteCodes, "CORRUPT",         INT2FIX(UNQLITE_CORRUPT));
  rb_define_const(mUnQLiteCodes, "LOCKED",          INT2FIX(UNQLITE_LOCKED));
  rb_define_const(mUnQLiteCodes, "BUSY",            INT2FIX(UNQLITE_BUSY));
  rb_define_const(mUnQLiteCodes, "DONE",            INT2FIX(UNQLITE_DONE));
  rb_define_const(mUnQLiteCodes, "PERM",            INT2FIX(UNQLITE_PERM));
  rb_define_const(mUnQLiteCodes, "NOTIMPLEMENTED",  INT2FIX(UNQLITE_NOTIMPLEMENTED));
  rb_define_const(mUnQLiteCodes, "NOTFOUND",        INT2FIX(UNQLITE_NOTFOUND));
  rb_define_const(mUnQLiteCodes, "NOOP",            INT2FIX(UNQLITE_NOOP)); // Used only for jx9 (unqlite v1.1.6)
  rb_define_const(mUnQLiteCodes, "INVALID",         INT2FIX(UNQLITE_INVALID));
  rb_define_const(mUnQLiteCodes, "EOF",             INT2FIX(UNQLITE_EOF));
  rb_define_const(mUnQLiteCodes, "UNKNOWN",         INT2FIX(UNQLITE_UNKNOWN));
  rb_define_const(mUnQLiteCodes, "LIMIT",           INT2FIX(UNQLITE_LIMIT));
  rb_define_const(mUnQLiteCodes, "EXISTS",          INT2FIX(UNQLITE_EXISTS));
  rb_define_const(mUnQLiteCodes, "EMPTY",           INT2FIX(UNQLITE_EMPTY));
  rb_define_const(mUnQLiteCodes, "COMPILE_ERR",     INT2FIX(UNQLITE_COMPILE_ERR)); // Used only for jx9 (unqlite v1.1.6)
  rb_define_const(mUnQLiteCodes, "VM_ERR",          INT2FIX(UNQLITE_VM_ERR)); // Used only for jx9 (unqlite v1.1.6)
  rb_define_const(mUnQLiteCodes, "FULL",            INT2FIX(UNQLITE_FULL));
  rb_define_const(mUnQLiteCodes, "CANTOPEN",        INT2FIX(UNQLITE_CANTOPEN));
  rb_define_const(mUnQLiteCodes, "READ_ONLY",       INT2FIX(UNQLITE_READ_ONLY));
  rb_define_const(mUnQLiteCodes, "LOCKERR",         INT2FIX(UNQLITE_LOCKERR));
}
