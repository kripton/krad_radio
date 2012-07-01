#!/usr/bin/env ruby

require_relative "kradradio.rb"

station_name = "ktv1"
playlist_dir = "/data2/youtube"
server1 = "phobos.kradradio.com 8088"
server2 = "phobos.kradradio.com 8080"

width = 640
height = 480
bitrate = 800

station = KradStation.new(station_name)

puts station.info
puts station.uptime

station.set_resolution(width, height)

station.cmd("bug 470 404 ~/Pictures/kr3.png")
sleep 0.2
station.cmd("transmit audiovideo #{server1} /ktv.webm secretkode null #{width} #{height} #{bitrate}")
sleep 0.2
station.cmd("transmit audio #{server2} /ktv.opus secretkode opus")
sleep 0.2
station.play_dir(playlist_dir)
