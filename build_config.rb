MRuby::Build.new do |conf|

  # Gets set by the VS command prompts.
  if ENV['VisualStudioVersion'] || ENV['VSINSTALLDIR']
    toolchain :visualcpp
  else
    toolchain :gcc
  end

  enable_debug

  # include the default GEMs
  conf.gembox 'default'

end

# Cross Compiling configuration for RX630
# http://gadget.renesas.com/
#
# Requires gnurx_v14.03
MRuby::CrossBuild.new("RX630") do |conf|
  toolchain :gcc

  # Linux
  BIN_PATH = "/usr/share/gnurx_v14.03_elf-1/bin"
  # BIN_PATH = "/Applications/IDE4GR.app/Contents/Java/hardware/tools/gcc-rx/rx-elf/rx-elf/bin"

  conf.cc do |cc|
    cc.command = "#{BIN_PATH}/rx-elf-gcc"
    cc.flags = "-Wall -g -Os -flto -mcpu=rx600 -m64bit-doubles"
    cc.compile_options = "%{flags} -o %{outfile} -c %{infile}"
  end

  conf.cxx do |cxx|
    cxx.command = conf.cc.command.dup
    cxx.include_paths = conf.cc.include_paths.dup
    cxx.flags = conf.cc.flags.dup
    cxx.defines = conf.cc.defines.dup
    cxx.compile_options = conf.cc.compile_options.dup
  end

  conf.linker do |linker|
    linker.command="#{BIN_PATH}/rx-elf-ld"
  end

  conf.archiver do |archiver|
    archiver.command = "#{BIN_PATH}/rx-elf-ar"
    archiver.archive_options = 'rcs %{outfile} %{objs}'
  end

  #no executables
  conf.bins = []

  #do not build executable test
  conf.build_mrbtest_lib_only

  #disable C++ exception
  conf.disable_cxx_exception

  #gems from core
  #conf.gem :core => "mruby-array-ext"
  #conf.gem :core => "mruby-bin-debugger"
  #conf.gem :core => "mruby-bin-mirb"
  #conf.gem :core => "mruby-bin-mrbc"
  #conf.gem :core => "mruby-bin-mruby"
  #conf.gem :core => "mruby-bin-mruby-config"
  #conf.gem :core => "mruby-bin-strip"
  #conf.gem :core => "mruby-class-ext"
  conf.gem :core => "mruby-compiler"
  #conf.gem :core => "mruby-enum-ext"
  #conf.gem :core => "mruby-enum-lazy"
  #conf.gem :core => "mruby-enumerator"
  #conf.gem :core => "mruby-error"
  #conf.gem :core => "mruby-eval"
  #conf.gem :core => "mruby-exit"
  #conf.gem :core => "mruby-fiber"
  #conf.gem :core => "mruby-hash-ext"
  #conf.gem :core => "mruby-inline-struct"
  #conf.gem :core => "mruby-kernel-ext"
  conf.gem :core => "mruby-math"
  # conf.gem :core => "mruby-numeric-ext"
  #conf.gem :core => "mruby-object-ext"
  #conf.gem :core => "mruby-objectspace"
  #conf.gem :core => "mruby-print"
  #conf.gem :core => "mruby-proc-ext"
  #conf.gem :core => "mruby-random"
  #conf.gem :core => "mruby-range-ext"
  #conf.gem :core => "mruby-sprintf"
  #conf.gem :core => "mruby-string-ext"
  #conf.gem :core => "mruby-struct"
  #conf.gem :core => "mruby-symbol-ext"
  #conf.gem :core => "mruby-test"
  #conf.gem :core => "mruby-time"
  #conf.gem :core => "mruby-toplevel-ext"

  #light-weight regular expression
  #conf.gem :github => "masamitsu-murase/mruby-hs-regexp", :branch => "master"


  # additional configrations for Arduino API
  conf.cc.flags << " -DGRSAKURA -DARDUINO=100 "
  conf.cc.include_paths << ["../gr_common/lib/", "../gr_common", "../gr_common/core","../gr_common/rx63n", "../gr_common/lib/SPI", "../gr_common/lib/Wire", "../gr_common/lib/Servo", "../gr_common/lib/SD" ]
  conf.cxx.flags = conf.cc.flags.dup
  conf.cxx.include_paths = conf.cc.include_paths.dup

  # Arduino API
  conf.gem :github => "takjn/mruby-arduino", :branch => "master"

  # Extention libraries for Gadget Renesas
  conf.gem :github => "takjn/mruby-gr-citrus", :branch => "master"

  # # NeoPixel library for mruby-arduino environment
  conf.gem :github => "takjn/mruby-arduino-neopixel", :branch => "master"

end
