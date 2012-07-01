#!/usr/bin/env ruby

require_relative "kradradio.rb"

server1 = "phobos.kradradio.com 8088"

width = 1280
height = 720
bitrate = 1200


station_name = "slideshowdemo"
station = KradStation.new (station_name)

puts station.info
puts station.uptime

station.set_resolution(width, height)

#station.cmd("transmit video #{server1} /slideshow.webm secretkode null #{width} #{height} #{bitrate}")
station.cmd("record video \"#{File.expand_path('~')}/Videos/slideshow.webm\" null #{width} #{height} #{bitrate}")

station.text("KRAD RADIO", -635, 315, 5, 114)
station.set_text(0, 25, 455, 15, 174, 1, 0)
sleep 0.2
station.set_text(0, 25, 455, 15, 174, 1, 0, 255, 11, 2)
sleep 1.5
station.text("Slide Show", 35, 655, 5, 94, 1, 0, 255, 255, 2, "LiberationSansNarrow")
sleep 3
station.set_text(0, 255, 455, 5, 4, 0, 0)
station.set_text(1, 1135, 655, 15, 94, 0, 0)
sleep 3
station.rm_text(0)
station.rm_text(1)

station.slideshow("#{File.expand_path('~')}/Pictures/slideshow", 2)

station.text("Thanks for watching", 135, 655, 5, 64, 1, 0, 125, 155, 155, "DroidSans")
sleep 1
station.set_text(0, 1535, -155, 15, 4, 0, -50)
sleep 0.3
station.text("See you next time, space cowboy!", 135, 655, 5, 64, 1, 0, 125, 155, 25, "Georgia")
station.set_text(1, 135, 655, 5, 64, 1, 0, 25, 255, 225)
sleep 1
station.set_text(0, 135, 655, 25, 64, 0, 0)
sleep 1
station.set_text(1, 135, 655, 25, 64, 1, 0, 255, 1, 1)
sleep 1
station.set_text(1, 135, 655, 25, 64, 1, 0, 255, 1, 1)
station.rm_text(0)
sleep 1
station.set_text(1, 135, 655, 25, 4, 0, 0, 255, 1, 1)
sleep 1
station.rm_text(1)
