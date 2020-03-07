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

    def test_append
      assert @db.store("key", "stored content")
      assert @db.append("key", " with appendix")
      assert_equal "stored content with appendix", @db.fetch("key")
    end

    def test_fetch
      @db.store("key", "wabba")

      assert_equal("wabba", @db.fetch("key"))
    end

    def test_fetchlonger
      @db.store("key", "wabbawabba")

      assert_equal("wabbawabba", @db.fetch("key"))
    end

    def test_store_multiple_uncommitted_with_failing_fetch_between
      @db.store("key1", "wabbauno")
      assert_raises UnQLite::NotFoundException do
        @db.fetch("key")
      end
      @db.store("key2", "wabbawabba")

      assert_equal("wabbauno", @db.fetch("key1"))
    end

    def test_store_multiple_committed_with_failing_fetch_between
      @db.store("key1", "wabbauno")
      @db.commit
      assert_raises UnQLite::NotFoundException do
        @db.fetch("key")
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

    def test_raw_string
      assert @db.store("key\x00value", "store\x00content")
      assert_equal "store\x00content", @db.fetch("key\x00value")
      assert_raises(UnQLite::NotFoundException) { @db.fetch("key") }
      assert_raises(UnQLite::NotFoundException) { @db.fetch("key\x00") }
      assert_raises(UnQLite::NotFoundException) { @db.fetch("key\x00value\x00") }
    end

    def test_each
      pairs = [ [ "alpha", "first" ], [ "beta", "second" ], [ "gamma", "third" ] ]
      [ :each, :each_pair ].each do |method_name|
        pairs.each { |pair| @db.store(*pair) }
        all = []
        @db.send(method_name) { |key, value| all << [key, value] }
        assert_equal pairs, all.sort_by { |k,v| k }
      end
    end

    def test_each_key
      pairs = [ [ "alpha", "first" ], [ "beta", "second" ], [ "gamma", "third" ] ]
      pairs.each { |pair| @db.store(*pair) }
      all = []
      @db.each_key { |key| all << key }
      assert_equal pairs.map { |k,v| k }, all.sort
    end

    def test_each_value
      pairs = [ [ "alpha", "first" ], [ "beta", "second" ], [ "gamma", "third" ] ]
      pairs.each { |pair| @db.store(*pair) }
      all = []
      @db.each_value { |value| all << value }
      assert_equal pairs.map { |k,v| v }, all.sort
    end

    def test_aref
      @db["key"] = "data"
      assert_equal "data", @db["key"]
      assert_equal "data", @db.fetch("key")
      assert_nil @db["nokey"]
    end

    def test_has_key
      @db.store "key", "value"
      [ :has_key?, :include?, :key?, :member? ].each do |nm|
        assert_equal true, @db.send(nm, "key")
        assert_equal false, @db.send(nm, "value")
      end
    end

    def test_closed?
      assert !@db.closed?
      @db.close
      assert @db.closed?
    end

    def test_clear
      @db.store "key", "value"
      @db.store "foo", "bar"
      @db.clear
      assert_raises(UnQLite::NotFoundException) { @db.fetch("key") }
      assert_raises(UnQLite::NotFoundException) { @db.fetch("foo") }
      @db.store "key", "value"
      assert_equal "value", @db.fetch("key")
    end

    def test_empty?
      assert @db.empty?
      @db.store "key", "value"
      assert !@db.empty?
    end

    def test_max_page_cache
      UnQLite::Database.open(db_path) do |db|
        db.max_page_cache = 1024
        db["key"] = "value"
      end
    end

    def test_kv_engine
      UnQLite::Database.open(db_path) do |db|
        assert db.kv_engine.kind_of?(String)
      end
    end

    # Not supports by unqlite as of version 1.1.6
    # def test_set_kv_engine
    #   UnQLite::Database.open(db_path) do |db|
    #     db.kv_engine = "hash"
    #   end
    # end
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

    def test_end_transaction_commit
      @db.begin_transaction
      @db.store "manual", "wabba"
      @db.end_transaction(true)
      assert_equal "wabba", @db.fetch("manual")
    end

    def test_end_transaction_rollback
      @db.begin_transaction
      @db.store "will_disappear", "wabba"
      @db.end_transaction(false)
      assert_raises(UnQLite::NotFoundException) { @db.fetch("will_disappear") }
    end

    def test_transaction
      @db.transaction do
        @db.store "alpha", "first"
        @db.store "beta", "second"
      end
      assert_equal "first", @db.fetch("alpha")
      assert_equal "second", @db.fetch("beta")
    end

    def test_transaction_failed
      assert_raises Exception do
        @db.transaction do
          @db.store "alpha", "first"
          @db.store "beta", "second"
          raise Exception
        end
      end
      assert_raises(UnQLite::NotFoundException) { @db.fetch("alpha") }
      assert_raises(UnQLite::NotFoundException) { @db.fetch("beta") }
    end

    def test_disable_auto_commit
      UnQLite::Database.open(db_path) do |db|
        db.disable_auto_commit
        db["key"] = "value"
      end
      UnQLite::Database.open(db_path) do |db|
        assert !db.include?("key")
      end
    end
  end

  class TestOpen < Minitest::Test
    attr_reader :db_path
    def setup
      @db_path = "#{Dir.mktmpdir("unqlite-ruby-test")}/db"
    end

    def teardown
      FileUtils.remove_entry(db_path) if File.exist?(db_path)
    end

    def test_open
      db = UnQLite::Database.open(db_path)
      db["key"] = "value"
      assert db.kind_of?(UnQLite::Database)
      assert !db.closed?
      db.close
    end

    def test_open_block
      UnQLite::Database.open(db_path) do |db|
        assert db.kind_of?(UnQLite::Database)
        assert !db.closed?
        db["key"] = "value"
      end
    end

    def test_open_create
      UnQLite::Database.open(db_path, UnQLite::CREATE) do |db|
        db["key"] = "value"
      end
      UnQLite::Database.open(db_path, UnQLite::READWRITE) do |db|
        assert_equal "value", db["key"]
      end
    end

    def test_open_readonly
      UnQLite::Database.open(db_path) do |db|
        db["key"] = "value"
      end
      UnQLite::Database.open(db_path, UnQLite::READONLY) do |db|
        assert_equal "value", db["key"]
        assert_raises(UnQLite::ReadOnlyException) { db["other"] = "something" }
      end
    end

    # VFS only?
    # def test_open_temp
    #   UnQLite::Database.open(db_path, UnQLite::TEMP_DB) do |db|
    #     db["key"] = "value"
    #   end
    #   assert !File.exist?(db_path)
    # end

    def test_open_readwrite
      UnQLite::Database.open(db_path) do |db|
        db["key"] = "value"
      end
      UnQLite::Database.open(db_path, UnQLite::READWRITE) do |db|
        assert_equal "value", db["key"]
      end
    end

    def test_open_readwrite_noexist
      assert_raises UnQLite::IOException do
        UnQLite::Database.open(db_path, UnQLite::READWRITE) do |db|
          db["key"] = "value"
        end
      end
    end

    def test_in_memory
      UnQLite::Database.open(db_path, UnQLite::IN_MEMORY | UnQLite::CREATE) do |db|
        db["key"] = "value"
      end
      assert !File.exist?(db_path)
    end

    def test_mmap
      UnQLite::Database.open(db_path) do |db|
        db["key"] = "value"
      end
      UnQLite::Database.open(db_path, UnQLite::READONLY | UnQLite::MMAP) do |db|
        assert_equal "value", db["key"]
        assert_raises(UnQLite::ReadOnlyException) { db["other"] = "something" }
      end
    end
  end
end
