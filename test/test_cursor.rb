require 'minitest/autorun'
require 'tmpdir'
require 'unqlite'

module UnQLite
  class CursorTest < Minitest::Test
    attr_reader :db_path, :db
    def setup
      @db_path = "#{Dir.mktmpdir("unqlite-ruby-test")}/db"
      @db = UnQLite::Database.open(@db_path)
    end

    def teardown
      db.close
      FileUtils.remove_entry(db_path) if File.exist?(db_path)
    end

    def test_key_value
      db.store "key", "value"
      cursor = UnQLite::Cursor.new(db)
      cursor.first!
      assert_equal "key", cursor.key
      assert_equal "value", cursor.value
    end

    def test_first_last
      db.store "alpha", "first"
      db.store "beta", "second"
      keys = []
      cursor = UnQLite::Cursor.new(db)
      cursor.first!
      keys << cursor.key
      cursor.last!
      keys << cursor.key
      assert_equal ["alpha", "beta"], keys.sort # order is undefined
    end

    def test_valid
      db.store "key", "value"
      cursor = UnQLite::Cursor.new(db)
      assert !cursor.valid?
      cursor.first!
      assert cursor.valid?
    end

    def test_first_next
      db.store "alpha", "first"
      db.store "beta", "second"
      db.store "gamma", "third"
      keys = []
      cursor = UnQLite::Cursor.new(db)
      cursor.first!
      while cursor.valid?
        keys << cursor.key
        cursor.next!
      end
      assert_equal ["alpha", "beta", "gamma"], keys.sort! # order is undefined
    end

    def test_last_prev
      db.store "alpha", "first"
      db.store "beta", "second"
      db.store "gamma", "third"
      keys = []
      cursor = UnQLite::Cursor.new(db)
      cursor.last!
      while cursor.valid?
        keys << cursor.key
        cursor.prev!
      end
      assert_equal ["alpha", "beta", "gamma"], keys.sort! # order is undefined
    end

    def test_seek
      db.store "alpha", "first"
      db.store "beta", "second"
      db.store "gamma", "third"
      cursor = UnQLite::Cursor.new(db)
      cursor.seek "beta"
      assert_equal "second", cursor.value
    end

    def test_delete
      db.store "alpha", "first"
      db.store "beta", "second"
      db.store "gamma", "third"
      cursor = UnQLite::Cursor.new(db)
      cursor.seek "beta"
      cursor.delete!
      assert_raises(UnQLite::NotFoundException) { @db.fetch "beta" }
    end
  end
end
