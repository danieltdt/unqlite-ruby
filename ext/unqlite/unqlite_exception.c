#include <unqlite_exception.h>

void rb_unqlite_raise(unqlite *db, int rc)
{
  VALUE klass = Qnil;

  switch(rc) {
    case UNQLITE_NOMEM:
      klass = rb_path2class("UnQLite::MemoryException");
      break;
    case UNQLITE_ABORT:
      klass = rb_path2class("UnQLite::AbortException");
      break;
    case UNQLITE_IOERR:
      klass = rb_path2class("UnQLite::IOException");
      break;
    case UNQLITE_CORRUPT:
      klass = rb_path2class("UnQLite::CorruptException");
      break;
    case UNQLITE_LOCKED:
      klass = rb_path2class("UnQLite::LockedException");
      break;
    case UNQLITE_BUSY:
      klass = rb_path2class("UnQLite::BusyException");
      break;
/* Not sure if it is an error or not (check lib/unqlite/errors.rb)
    case UNQLITE_DONE:
      klass = rb_path2class("UnQLite::DoneException");
      break;
*/
    case UNQLITE_PERM:
      klass = rb_path2class("UnQLite::PermissionException");
      break;
    case UNQLITE_NOTIMPLEMENTED:
      klass = rb_path2class("UnQLite::NotImplementedException");
      break;
    case UNQLITE_NOTFOUND:
      klass = rb_path2class("UnQLite::NotFoundException");
      break;
    case UNQLITE_EMPTY:
      klass = rb_path2class("UnQLite::EmptyException");
      break;
    case UNQLITE_INVALID:
      klass = rb_path2class("UnQLite::InvalidParameterException");
      break;
    case UNQLITE_EOF:
      klass = rb_path2class("UnQLite::EOFException");
      break;
    case UNQLITE_UNKNOWN:
      klass = rb_path2class("UnQLite::UnknownConfigurationException");
      break;
    case UNQLITE_LIMIT:
      klass = rb_path2class("UnQLite::LimitReachedException");
      break;
    case UNQLITE_FULL:
      klass = rb_path2class("UnQLite::FullDatabaseException");
      break;
    case UNQLITE_CANTOPEN:
      klass = rb_path2class("UnQLite::CantOpenDatabaseException");
      break;
    case UNQLITE_READ_ONLY:
      klass = rb_path2class("UnQLite::ReadOnlyException");
      break;
    case UNQLITE_LOCKERR:
      klass = rb_path2class("UnQLite::LockProtocolException");
      break;
  }

  if( !NIL_P(klass) ) { // Is really an error?
    const char *buffer;
    int length;

    // Try to get error from log (though only available on commit,
    // rollback, store & append).
    unqlite_config(db, UNQLITE_CONFIG_ERR_LOG, &buffer, &length);

    // Raise it!
    if( length > 0 )
      rb_raise(klass, "%s", buffer);
    else {
       VALUE klass_name = rb_class_name(klass);
       rb_raise(klass, "%s", StringValueCStr(klass_name));
    }
  }
}
