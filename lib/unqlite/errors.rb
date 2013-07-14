module UnQLite
  class Exception < ::StandardError
    @code = 0
    @errors = []

    def self.inherited(subclass)
      @errors.push subclass
    end

    def self.code
      @code
    end
    def code; self.class.code; end # Helper for .code

    def self.for_code(rc)
      @errors.find { |e| e.code == rc }
    end
  end

  class MemoryException < Exception
    @code = -1
  end

  class AbortException < Exception
    @code = -10
  end

  class IOException < Exception
    @code = -2
  end

  class CorruptException < Exception
    @code = -24
  end

  class LockedException < Exception # Forbidden operation
    @code = -4
  end

  class BusyException < Exception
    @code = -14
  end

  #class DoneException < Exception; end # It is not clear on docs if it's an error or not

  class PermissionException < Exception
    @code = -19
  end

  class NotImplementedException < Exception
    @code = -17
  end

  class NotFoundException < Exception # It is a quite confusing if is an error or an acceptable behavior (unqlite v1.1.6)
    @code = -6
  end

  class EmptyException < Exception # Empty key (and some jx9 functions)
    @code = -3
  end

  class InvalidParameterException < Exception
    @code = -9
  end

  class EOFException < Exception
    @code = -18
  end

  class UnknownConfigurationException < Exception
    @code = -18
  end

  class LimitReachedException < Exception
    @code = -13
  end

  class FullDatabaseException < Exception
    @code = -73
  end

  class CantOpenDatabaseException < Exception
    @code = -74
  end

  class ReadOnlyException < Exception
    @code = -75
  end

  class LockProtocolException < Exception
    @code = -76
  end
end
