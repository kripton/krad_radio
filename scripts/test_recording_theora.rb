#!/usr/bin/env ruby

require "/home/oneman/kode/krad_radio/scripts/kradradio_client.rb"

station_name = "kradtest"

station = KradStation.new(station_name)


station.cmd("setdir ~/kradtoday")
logname = station.cmd("logname")
station.cmd("res 960 540")
#station.cmd("fps 30")
sleep 1
station.cmd("capture x11")
#station.cmd("addsprite ~/Pictures/anim0316-1_e0_frames_6.png 220 220")
station.cmd("record video ~/Videos/kradtest_theora42.ogg theora")
sleep 1

#320x240.ogg  320x240.ogv  322x242_not-divisible-by-sixteen-framesize.ogg  chained_streams.ogg  ducks_take_off_444_720p25.ogg  mobile_itu601_i_422.ogg  multi2.ogg
#station.cmd("play ~/Videos/theora_test/ducks2.ogg")
#station.cmd("play ~/Videos/krad_masked.webm")
#station.cmd("play ~/Videos/1280x720.ogv")
#station.cmd("capture test")
station.cmd("capture x11")
sleep 2
puts station.cmd("ls")
station.cmd("setsprite 0 120 120 3 3")
sleep 12
station.cmd("rm 1")
sleep 2
station.cmd("rm 0")
sleep 2
station.cmd("kill")

puts `tail -n 5000 #{logname}`
