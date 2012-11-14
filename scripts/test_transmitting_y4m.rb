#!/usr/bin/env ruby

require "/home/oneman/kode/krad_radio/scripts/kradradio_client.rb"

station_name = "kradtest"

station = KradStation.new(station_name)


station.cmd("setdir ~/kradtoday")
logname = station.cmd("logname")
station.cmd("res 1280 720")
sleep 1
station.cmd("transmitter_on 3030")
station.cmd("addsprite ~/Pictures/anim0316-1_e0_frames_6.png 320 320")
station.cmd("transmit video transmitter 3030 /#{station_name}.y4m nopass y4m")
sleep 2
station.cmd("ls")

while true do
  station.cmd("setsprite 0 620 420 3 3")
  sleep 5
  station.cmd("setsprite 0 120 520 3 2")
  sleep 5  
end

station.cmd("rm 0")
sleep 1
station.cmd("kill")

puts `tail -n 5000 #{logname}`
