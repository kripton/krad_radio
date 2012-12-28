#!/usr/bin/env ruby

# converting a animated gif to a png sprite sheet

dtype = "Previous"
dtype2 = "None"
dtype3 = "Undefined"

input = ARGV[0]
output = input.sub(".gif", "_fixed.gif")

#puts input + " -> " + output

cmd = "convert -despeckle #{input} #{output}"

#puts cmd
#`#{cmd}`


input = "/home/oneman/Pictures/test/outy*.png"
frames = `identify #{input} | wc -l`.chomp
output2 = input.sub(".gif", "").sub("*", "x") + "_frames_" + frames + ".png"
#output2 = output.sub(".gif", "") + "_frames_" + frames + ".tiff"

puts input + " -> " + output2

cmd = "montage -background transparent #{input} -set label '%[fx:t+1]' -tile 10x -geometry '1x1+0+0<' -background transparent -quality 100 #{output2}"

puts cmd

`#{cmd}`
