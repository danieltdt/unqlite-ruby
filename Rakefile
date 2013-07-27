require "bundler/gem_tasks"
require 'rake/extensiontask'
require 'rake/testtask'

Rake::ExtensionTask.new('unqlite_native') do |ext|
  ext.ext_dir = 'ext/unqlite'
  ext.lib_dir = 'lib/unqlite'
end

Rake::TestTask.new do |t|
  t.libs << "test"
  t.test_files = FileList['test/test*.rb']
  t.verbose = true
end
