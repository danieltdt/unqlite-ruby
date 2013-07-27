require "bundler/gem_tasks"
require 'rake/extensiontask'

Rake::ExtensionTask.new('unqlite_native') do |ext|
  ext.ext_dir = 'ext/unqlite'
  ext.lib_dir = 'lib/unqlite'
end

task :test => :compile do
  sh "ruby", "-rminitest/autorun", "-Ilib", "-Itest", *Dir.glob("test/test_*.rb")
end
