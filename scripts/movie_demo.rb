#!/usr/bin/env ruby

require_relative "kradradio.rb"


station_name = "moviedemo"
station = KradStation.new (station_name)

puts station.info
puts station.uptime
