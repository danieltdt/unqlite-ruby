# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'unqlite/version'

Gem::Specification.new do |spec|
  spec.name          = "unqlite"
  spec.version       = UnQLite::VERSION
  spec.authors       = ["Daniel Teixeira"]
  spec.email         = ["daniel.t.dt@gmail.com"]
  spec.description   = %q{UnQLite lib for ruby, using C extension.}
  spec.summary       = %q{UnQLite for ruby (using C extension)}
  spec.homepage      = "https://github.com/danieltdt/unqlite-ruby"
  spec.license       = "MIT"

  spec.files         = `git ls-files`.split($/)
  spec.extensions    = ['ext/unqlite/extconf.rb']
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler",        "~> 2.2.10"
  spec.add_development_dependency "rake",           "~> 13.0.1"
  spec.add_development_dependency "ZenTest",        "~> 4.12.0"
  spec.add_development_dependency "rake-compiler",  "~> 1.1.0"
  spec.add_development_dependency "minitest",       "~> 5.14.0"
end
