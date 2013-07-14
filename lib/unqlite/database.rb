module UnQLite
  class Database
    def native
      UnQLite::Native::KVStoreAPI
    end

    def initialize(path_or_memory, mode = :UNQLITE_OPEN_CREATE)
      @handler = native.create_handler

      native.unqlite_open(@handler.ptr, path_or_memory, mode)
    end

    def close
      check_handler

      native.unqlite_close(@handler.ptr)
    end

    def store(key, value)
      check_handler
      key = key.to_s

      native.unqlite_kv_store @handler.ptr, key, key.size, value.to_str, value.to_str.size
    end

    def fetch(key)
      check_handler
      key = key.to_s

      content_buffer = FFI::MemoryPointer.new(:void)
      content_size_buffer = FFI::MemoryPointer.new(:size_t)

      native.unqlite_kv_fetch @handler.ptr, key, key.size, content_buffer, content_size_buffer

      length = content_size_buffer.read_string.to_i
      content_buffer.read_string_length length
    end

    def delete(key)
      native.unqlite_kv_delete @handler.ptr, key, key.size
    end

    private

    def check_handler
      raise "No handler." if @handler.nil?
    end
  end
end
