#!/usr/bin/env ruby

rot = 0.1

while true do
	cmd = "krad_radio ktv1 setsprite 0 715 15 3 2.5 1.0 #{rot}"
	puts cmd
	`#{cmd}`
	sleep 0.05
	rot += 0.8
end
