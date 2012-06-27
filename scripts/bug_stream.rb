#!/usr/bin/env ruby

def bug_stream (station)

	stacmd = "krad_radio #{station}"

	logo1 = "~/Pictures/kr3.png"
	logo2 = "~/Pictures/krad_radio_tv_logo_pixel.png"
	logo3 = "~/Pictures/krad_radio_tv_logo_blur.png"
	
	while true do

		`#{stacmd} bug 1100 620  #{logo1}`
		sleep 20
		`#{stacmd} bug 1100 620 ""`
		sleep 20

		`#{stacmd} bug 13 320 #{logo2}`	
		sleep 7
		`#{stacmd} bug 13 320 ""`
		sleep 50

		`#{stacmd} bug 13 320 #{logo3}`
		sleep 7
		`#{stacmd} bug 13 320 ""`
		sleep 50
	end
end
