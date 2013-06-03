require 'helper'

module UnQLite
  class TestDatabase < Minitest::Test
    attr_reader :db

    def setup
      @db = UnQLite::Database.new(":mem:")
    end

    def test_store
      assert_equal UnQLite::Codes::OK, @db.store("key", "stored content")
    end

    def test_fetch
      @db.store("key", "wabba")

      assert_equal "wabba", @db.fetch("key")
    end

    def test_fetch_unexisting_key
      assert_raises(UnQLite::NotFoundException) { @db.fetch("xxx") }
    end
  end
end
