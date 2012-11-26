#!/usr/bin/env ruby

require_relative "kradradio_client.rb"

width = 1280
height = 720
bitrate = 1400

station_name = "moviedemo"
station = KradStation.new(station_name)

puts station.info
puts station.uptime

station.cmd("record video \"#{File.expand_path('~')}/Videos/textdemo.webm\"")

station.text("KRAD RADIO", 25, -315, 5, 114)
station.set_text(0, 25, 455, 15, 174, 1, 0)
sleep 0.2
station.set_text(0, 25, 455, 15, 174, 1, 0, 255, 11, 2)
sleep 1.5
station.text("Text Text Text", 35, 655, 5, 94, 1, 0, 155, 255, 222, "LiberationSans")
sleep 3
station.set_text(0, 255, 455, 5, 4, 1, 555)
station.set_text(1, 1135, 655, 15, 394, 1, -555)
sleep 3
station.set_text(0, 255, 455, 5, 4, 0, 0)
station.set_text(1, 1135, 655, 15, 94, 0, 0)
sleep 0.5
station.rm_text(0)
station.rm_text(1)

station.sprite("/home/oneman/Pictures/animated/angif_25_fixed_frames_30.png", 470, 304)

station.set_sprite(0, 333, 155, 5, 4.5, 1, 25)
sleep 1
station.set_sprite(0, 33, 65, 6, 4.5, 1, 325)
sleep 2
station.set_sprite(0, 533, 235, 7, 4.5, 1, 5)
sleep 3
station.throw_sprite(0)
sleep 2
station.rm_sprite(0)
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
sleep 1
station.cmd("rm 0")
sleep 1
station.cmd("kill")
