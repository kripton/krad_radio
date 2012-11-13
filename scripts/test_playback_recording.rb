#!/usr/bin/env ruby

require "/home/oneman/kode/krad_radio/scripts/kradradio_client.rb"

station_name = "kradtest"

station = KradStation.new(station_name)


station.cmd("setdir ~/kradtoday")
logname = station.cmd("logname")
station.cmd("res 1280 720")
sleep 1
station.cmd("addsprite ~/Pictures/anim0316-1_e0_frames_6.png 320 320")
station.cmd("record audiovideo ~/Videos/kradtest2.webm vp8vorbis 1280 720 2200 0.5")
sleep 1
station.cmd("play ~/Videos/krad_masked.webm")
sleep 10
station.cmd("rm 1")
sleep 1
station.cmd("rm 0")
sleep 1
station.cmd("kill")

puts `tail -n 5000 #{logname}`
