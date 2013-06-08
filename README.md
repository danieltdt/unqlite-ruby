# UnQLite for Ruby

[UnQLite](http://www.unqlite.org/) interface for ruby programs.

Note: This is an alpha version and many features are missing! But,
I would love to merge some pull-requests to make it better (:

## Installation

You have to install UnQLite into your system and compile it as a shared library. Unfortunately, 
UnQLite doesn't have a Makefile (or something like that) to automate that step. If you are on
linux, you can check [this gist](https://gist.github.com/danieltdt/5693070) and compile it using gcc.

After installing UnQLite, add this line to your application's Gemfile:

    gem 'unqlite'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install unqlite

## Usage

For in-memory databases

    db = UnQLite::Database.new(":mem:")
    db.store("key", "wabba")
    db.fetch("key") # => "wabba"
    db.close

For regular databases

    db = UnQLite::Database.new("database.db") # You may also give a full path
    db.store("key", "wabba")
    db.fetch("key") # => "wabba"

    # Now you have to commit your changes or close your database
    db.commit

    db.store("key2", "wabba2")
    db.close # Will automatically commit


## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request
