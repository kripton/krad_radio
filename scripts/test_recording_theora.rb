#!/usr/bin/env ruby

require "/home/oneman/kode/krad_radio/scripts/kradradio_client.rb"

station_name = "kradtest"

station = KradStation.new(station_name)


station.cmd("setdir ~/kradtoday")
logname = station.cmd("logname")
station.cmd("res 1280 720")
sleep 1
station.cmd("addsprite ~/Pictures/anim0316-1_e0_frames_6.png 320 320")
station.cmd("record video ~/Videos/kradtest_theora.ogg theora 640 360")
sleep 1
puts station.cmd("ls")
station.cmd("setsprite 0 120 120 3 3")
sleep 8
station.cmd("rm 1")
sleep 2
station.cmd("rm 0")
sleep 2
station.cmd("kill")

puts `tail -n 5000 #{logname}`
