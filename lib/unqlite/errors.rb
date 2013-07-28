module UnQLite
  class Exception < ::StandardError
    @code = 0

    def self.code
      @code
    end

    def code
      self.class.code
    end
  end

  class MemoryException < Exception; end
  class AbortException < Exception; end
  class IOException < Exception; end
  class CorruptException < Exception; end
  class LockedException < Exception; end # Forbidden operation
  class BusyException < Exception; end
  #class DoneException < Exception; end # It is not clear on docs if it's an error or not
  class PermissionException < Exception; end
  class NotImplementedException < Exception; end
  class NotFoundException < Exception; end # It is a quite confusing if is an error or an acceptable behavior (unqlite v1.1.6)
  class EmptyException < Exception; end # Empty key (and some jx9 functions)
  class InvalidParameterException < Exception; end
  class EOFException < Exception; end
  class UnknownConfigurationException < Exception; end
  class LimitReachedException < Exception; end
  class FullDatabaseException < Exception; end
  class CantOpenDatabaseException < Exception; end
  class ReadOnlyException < Exception; end
  class LockProtocolException < Exception; end
  class UnsupportedException < Exception; end
end
