#!/usr/bin/env ruby

# http://nick.onetwenty.org/index.php/2011/01/21/animated-gif-to-sprite-sheet-using-imagemagick/

input = ARGV[0]
output = input.sub(".gif", "_fixed.gif")

puts input + " -> " + output

cmd = "convert -layers dispose #{input} #{output}"

puts cmd
`#{cmd}`


