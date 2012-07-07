#!/usr/bin/env ruby

require_relative "kradradio_client.rb"

station_name = "radio_test"
station = KradStation.new (station_name)

puts station.info

station.cmd("input Music1")
station.cmd("input Music2")
station.cmd("output Main")
station.cmd("xfade Music1 Music2")

`jack_connect XMMS2:out_1 #{station_name}:Music1_Left`
`jack_connect XMMS2:out_2 #{station_name}:Music1_Right`

`jack_connect XMMS2-01:out_1 #{station_name}:Music2_Left`
`jack_connect XMMS2-01:out_2 #{station_name}:Music2_Right`

sleep 1

#station.cmd("capture v4l2")
station.cmd("transmitav phobos.kradradio.com 8080 /#{station_name}.webm secretkode")
#station.cmd("vuon")
sleep 2
station.cmd("webon 13000 13001")
station.cmd("transmit audio phobos.kradradio.com 8080 /#{station}.opus secretkode opus")

