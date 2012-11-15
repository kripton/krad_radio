#!/usr/bin/env ruby

# converting a animated gif to a png sprite sheet

input = ARGV[0]
frames = `identify #{input} | wc -l`.chomp
output = input.sub(".gif", "") + "_frames_" + frames + ".png"

puts input + " -> " + output

cmd = "montage #{input} -tile 10x -geometry '1x1+0+0<' -background \"rgba(0, 0, 0, 0.0)\" -quality 100 #{output}"

puts cmd

`#{cmd}`
