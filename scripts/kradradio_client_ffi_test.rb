#!/usr/bin/env ruby

require 'ffi'

module KradRadioClientBinding
  extend FFI::Library
  ffi_lib "/usr/local/lib/libkradradio_client.so"
  attach_function :kr_connect, [:string], :pointer
  attach_function :krad_ipc_radio_uptime, [:pointer], :void
  attach_function :krad_ipc_radio_get_system_info, [:pointer], :void
  attach_function :krad_ipc_print_response, [:pointer], :void  
  attach_function :kr_disconnect, [:pointer], :void
end

class KradStation

  def initialize(name_or_url)
    @name = name_or_url
    @station = KradRadioClientBinding.kr_connect(@name)
    if @station.null?
      puts "Station #{@name} is not running!"
      Kernel.exit
    end
    Kernel.srand
  end

  def print_uptime
    KradRadioClientBinding.krad_ipc_radio_uptime(@station)
    KradRadioClientBinding.krad_ipc_print_response(@station)
  end

  def print_info
    KradRadioClientBinding.krad_ipc_radio_get_system_info(@station)
    KradRadioClientBinding.krad_ipc_print_response(@station)
  end

  def disconnect()
    KradRadioClientBinding.kr_disconnect(@station)
    @station = 0
  end

end


def time_diff_milli(start, finish)
   (finish - start) * 1000.0
end

def test_old_way(station_name, test_count=1000)
  puts "Testing old way"
  start = Time.new
  test_count.times do
    #puts `krad_radio #{station_name} uptime`.chomp
    `krad_radio #{station_name} uptime`.chomp    
  end
  finish = Time.new
  msecs = time_diff_milli(start, finish)
  return msecs.to_s + "ms"
end

def test_new_way(station_name, test_count=1000)
  puts "Testing new way"
  station = KradStation.new(station_name)
  start = Time.new
  test_count.times do
    station.print_uptime
  end
  finish = Time.new
  msecs = time_diff_milli(start, finish)
  station.disconnect
  return msecs.to_s + "ms"
end


station_name = "monkey"
test_count = 1000

oldtime = test_old_way(station_name, test_count)
newtime = test_new_way(station_name, test_count)
puts "\n\n"
sleep 0.1
puts "Test Count: " + test_count.to_s

puts "Old way time: " + oldtime
puts "New way time: " + newtime
