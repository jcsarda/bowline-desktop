raise "Only supports Ruby 1.9.1" if RUBY_VERSION !~ /^1\.9/

gem "bowline"
require "bowline/platform"
include Bowline::Platform

require "rbconfig"
gem "rice", ">= 1.3.0"

rice_gem = Gem.cache.find_name("rice").first
raise "Can't find rice gem" unless rice_gem

includes = []
includes << RbConfig::CONFIG['rubyhdrdir']
includes << File.join(RbConfig::CONFIG['rubyhdrdir'], RUBY_PLATFORM)
includes << File.join(rice_gem.full_gem_path, *%w{ruby lib include})
includes << "."

opts = includes.map {|inc| "-I#{inc}" }
opts << "`wx-config --cxxflags`"
opts << "`pkg-config --cflags webkit-1.0`" if linux?
opts << "-Flibs"

libs = []

# Dynamic libs
libs << "`pkg-config --libs webkit-1.0`" if linux?
libs << "-Flibs"
libs << "-L."
libs << "-lcrypt" if linux?
if osx?
  libs << "-framework JavaScriptCore"
  libs << "-framework WebKit"
end

# Static libs
libs << "`wx-config --libs`"
libs << RbConfig::CONFIG['LDFLAGS']
libs << RbConfig::CONFIG['LIBRUBYARG_STATIC']
libs << "-L" + File.join(rice_gem.full_gem_path, *%w{ruby lib lib})
libs << "-lrice"

libs << "-Wl,-rpath,@loader_path/libs -Wl,-rpath,@loader_path/../Frameworks  -Wl,-rpath,@loader_path/../Libraries"

debug_flags      = "-g -Wall -Wcast-align -Wmissing-noreturn -Wundef"
standard_flags   = "-fmessage-length=0 -Wno-trigraphs"

if osx?
  standard_flags += " -arch i386 -fpascal-strings -mmacosx-version-min=10.5 -isysroot /Developer/SDKs/MacOSX10.5.sdk"
else
  standard_flags += " -march=i386"
end

vars :CC => "g++", :FLAGS => [debug_flags, standard_flags], :LIBS => libs, :OPTS => opts

all_depends = []
all_depends << "badge_label.o" if osx?
all_depends << "bowline_webkit.o"
all_depends << "main.o"

rule :all, :depends => all_depends do
  compile :LIBS, :output => "bowline-desktop"
  echo "Done compiling, run ./bowline-desktop"
end

rule "main.o", :depends => ["main.cpp"] do
  compile :to_obj, :OPTS, :$@
end

rule "badge_label.o", :depends => ["bowline/badge_label.h", "bowline/badge_label.mm"] do
  compile :to_obj, :OPTS, :$@
end

bowline_webkit_depends = []
bowline_webkit_depends << "bowline/bowline_webkit.h"

bowline_webkit_depends << begin
  case type
  when :osx   then "bowline/bowline_webkit_mac.mm"
  when :linux then "bowline/bowline_webkit_gtk.cpp"
  when :win   then "bowline/bowline_webkit_win.cpp"
  else
    raise "Unknown platform"
  end
end

rule "bowline_webkit.o", :depends => bowline_webkit_depends do
  compile :to_obj, :OPTS, :$@
end

rule :test, :depends => ["test.cpp"] do
  compile :LIBS, :OPTS, :$@
end

clean "*.o", "bowline-desktop", "test"
