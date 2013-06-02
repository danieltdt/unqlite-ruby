require 'mkmf'

abort "unqlite.h is missing. Please, install unqlite." unless find_header 'unqlite.h'
abort "unqlite is missing. Please, install unqlite" unless find_library 'unqlite', 'unqlite_open'

create_makefile('unqlite/unqlite_native')
