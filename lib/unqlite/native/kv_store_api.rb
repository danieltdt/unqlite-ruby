module UnQLite
  module Native
    module KVStoreAPI
      extend FFI::Library

      ffi_lib 'unqlite'

      # Error checking
      def self.attach_function(*args)
        method_name = args.first
        method_name_for_original = "#{method_name}_without_error_checking"

        super(*args)
        alias_method(method_name_for_original, method_name)

        define_method method_name do |*method_args, &block|
          public_send(method_name_for_original.to_sym, *method_args, &block).tap do |rc|
            exception = UnQLite::Exception.for_code(rc)

            raise exception.new if exception
          end
        end

        module_function method_name
        module_function method_name_for_original
      end

      class EngineHandler
        extend FFI::DataConverter
        native_type FFI::Type::POINTER

        def initialize(pointer)
          @handler_pointer = pointer
        end

        def ptr
          @handler_pointer
        end

        def self.from_native(value, ctx)
          new(value)
        end

        def self.to_native(handler, ctx)
          handler.ptr
        end

        def self.size
          FFI.type_size(:pointer)
        end
      end

      typedef :pointer, EngineHandler

      enum :connection_mode, [
        :UNQLITE_OPEN_CREATE,    0x00000004,
        :UNQLITE_OPEN_READWRITE, 0x00000002,
        :UNQLITE_OPEN_READONLY,  0x00000001,
        :UNQLITE_OPEN_MMAP,      0x00000100,
      # :UNQLITE_OPEN_TEMP,      0x0, # Not in .h or .c
      # :UNQLITE_OPEN_MEM,       0x0, # Not in .h or .c
      # :UNQLITE_OPEN_NO_MUTEX,  0x0, # Not in .h or .c
        :UNQLITE_OPEN_OMIT_JOURNALING, 0x00000040
      ]

      attach_function :unqlite_open, [EngineHandler, :string, :connection_mode], :int
      attach_function :unqlite_close, [EngineHandler], :int

      attach_function :unqlite_kv_store, [EngineHandler, :string, :int, :pointer, :long_long], :int
      attach_function :unqlite_kv_fetch, [EngineHandler, :string, :int, :buffer_inout, :buffer_inout], :int
      attach_function :unqlite_kv_delete, [EngineHandler, :string, :int], :int

      attach_function :unqlite_begin, [EngineHandler], :int
      attach_function :unqlite_commit, [EngineHandler], :int
      attach_function :unqlite_rollback, [EngineHandler], :int

      def self.create_handler
        EngineHandler.new FFI::MemoryPointer.new(:void)
      end
    end
  end
end
