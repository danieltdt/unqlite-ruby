require 'helper'

module UnQLite
  class TestErros < Minitest::Test
    attr_reader :exceptions

    def setup
      @exceptions = [
        UnQLite::MemoryException,
        UnQLite::AbortException,
        UnQLite::IOException,
        UnQLite::CorruptException,
        UnQLite::LockedException,
        UnQLite::BusyException,
        UnQLite::PermissionException,
        UnQLite::NotImplementedException,
        UnQLite::NotFoundException,
        UnQLite::EmptyException,
        UnQLite::InvalidParameterException,
        UnQLite::EOFException,
        UnQLite::UnknownConfigurationException,
        UnQLite::LimitReachedException,
        UnQLite::FullDatabaseException,
        UnQLite::CantOpenDatabaseException,
        UnQLite::ReadOnlyException,
        UnQLite::LockedException
      ] # + [UnQLite::DoneException]
    end

    def test_respond_to_code
      exceptions.each do |exception|
        assert true, exception.respond_to?(:code)
      end
    end
  end
end
