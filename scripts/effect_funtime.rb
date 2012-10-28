#!/usr/bin/env ruby

require "/home/oneman/kode/krad_radio/scripts/kradradio_client.rb"

station_name = "kgarden"

station = KradStation.new(station_name)

#station.cmd("addfx Music pass")
#station.cmd("setfx Music 0 type 0 0")
station.cmd("setfx Music 0 bandwidth 0 1")


station.cmd("setfx Music 0 type 0 0")

f = 80

while f < 18000
  station.cmd("setfx Music 0 hz 0 #{f}")
  f += 15
  if f > 5000
    f += 50
  end
  sleep 0.01
end

station.cmd("setfx Music 0 type 0 0")

f = 18000

while f > 50
  station.cmd("setfx Music 0 hz 0 #{f}")
  f -= 50
  if f < 5000
    f += 35
  end
  sleep 0.01
end
