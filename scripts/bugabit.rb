#!/usr/bin/env ruby

station = "radio1"

stacmd = "krad_radio #{station}"

while true do

	`#{stacmd} bug 470 404 ~/Pictures/kr3.png`
	sleep 20
	`#{stacmd} bug 470 404 ""`
	sleep 20

	`#{stacmd} bug 13 320 ~/Pictures/krad_radio_tv_logo_pixel.png`	
	sleep 7
	`#{stacmd} bug 13 320 ""`
	sleep 50

	`#{stacmd} bug 13 320 ~/Pictures/krad_radio_tv_logo_blur.png`
	sleep 7
	`#{stacmd} bug 13 320 ""`
	sleep 50



end

