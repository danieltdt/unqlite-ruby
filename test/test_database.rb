require 'tempfile'
require 'helper'

module UnQLite
  module CommonTestsForDatabase
    def self.included(klass)
      klass.class_eval { attr_reader :db_path }
    end

    def setup
      @db = UnQLite::Database.new(db_path)
    end

    def teardown
      @db.close
    end

    def test_store
      assert @db.store("key", "stored content")
    end

    def test_fetch
      @db.store("key", "wabba")

      assert_equal("wabba", @db.fetch("key"))
    end

    def test_fetchlonger
      @db.store("key", "wabbawabba")

      assert_equal("wabbawabba", @db.fetch("key"))
    end

    ## TODO: maybe change the API around fetch() so it e.g. returns bool false instead raising an exception, feels weird to use exception handling block in testcases.
    def test_store_multiple_uncommitted_with_failing_fetch_between
      @db.store("key1", "wabbauno")
      begin
        ## the fetch will fail because 'key" doesn't exist and in turn this will trigger a db rollback which erases all data that was not comitted yet.
        @db.fetch("key")
      rescue => e
        ;
      end
      @db.store("key2", "wabbawabba")

      assert_equal("wabbauno", @db.fetch("key1"))
    end

    def test_store_multiple_committed_with_failing_fetch_between
      @db.store("key1", "wabbauno")
      @db.commit
      begin
        ## the fetch will fail because 'key" doesn't exist but we won't lose any data because everything before was already committed
        @db.fetch("key")
      rescue => e
        ;
      end
      @db.store("key2", "wabbawabba")

      assert_equal("wabbauno", @db.fetch("key1"))
    end

    def test_store_fetch_multiple
      @db.store("key1", "wabbauno")
      @db.store("key2", "wabbawabba")

      assert_equal("wabbauno", @db.fetch("key1"))
      assert_equal("wabbawabba", @db.fetch("key2"))
    end

    def test_delete
      @db.store("key", "wabba")
      assert_equal("wabba", @db.fetch("key"))
      @db.delete("key")
      assert_raises(UnQLite::NotFoundException) { @db.fetch("key") }
    end

    def test_exceptions
      # TODO: Test other errors
      assert_raises(UnQLite::NotFoundException) { @db.fetch("xxx") }
    end
  end

  class TestInMemoryDatabase < Minitest::Test
    include CommonTestsForDatabase

    def initialize(*args)
      @db_path = ":mem:"
      super(*args)
    end
  end

  class TestDatabase < Minitest::Test
    include CommonTestsForDatabase

    def setup
      @tmp = Tempfile.new("test_db")
      @db_path = @tmp.path
      @tmp.close
      super()
    end

    def teardown
      super()
      @tmp.unlink
    end

    def test_automatic_transaction
      @db.store("auto", "wabba")
      @db.rollback

      assert_raises(UnQLite::NotFoundException) { @db.fetch("auto") }
    end

    def test_manual_transaction
      @db.begin_transaction
      @db.store("manual", "wabba")
      @db.rollback

      assert_raises(UnQLite::NotFoundException) { @db.fetch("manual") }

      @db.store("manual2", "wabba")
      @db.commit
      @db.store("will_disapper", "wabba")
      @db.rollback

      assert_equal("wabba", @db.fetch("manual2"))
      assert_raises(UnQLite::NotFoundException) { @db.fetch("will_disapper") }
    end
  end
end
