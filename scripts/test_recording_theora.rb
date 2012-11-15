#!/usr/bin/env ruby

require "/home/oneman/kode/krad_radio/scripts/kradradio_client.rb"

station_name = "kradtest"

station = KradStation.new(station_name)


station.cmd("setdir ~/kradtoday")
logname = station.cmd("logname")
station.cmd("res 640 360")
station.cmd("fps 60")
sleep 1
station.cmd("addsprite ~/Pictures/anim0316-1_e0_frames_6.png 220 220")
station.cmd("record audiovideo ~/Videos/kradtest_theora29.ogg theoravorbis")
sleep 1
#station.cmd("play ~/Videos/krad_masked.webm")
station.cmd("play ~/Videos/1280x720.ogv")
#station.cmd("capture test")
#station.cmd("capture x11")
sleep 2
puts station.cmd("ls")
station.cmd("setsprite 0 120 120 3 3")
sleep 8
station.cmd("rm 1")
sleep 2
station.cmd("rm 0")
sleep 2
station.cmd("kill")

puts `tail -n 5000 #{logname}`
