#!/usr/bin/env bash


export width=1280
export height=720
export bitrate=2000
export HTTP_PORT=13002
export WEBSOCKET_PORT=13003
export station_name="moviedemo"

krad_radio $station_name launch
sleep 1.0
krad_radio $station_name setdir ~/krad_stations
sleep 1.0
konsole -e tail -n1000 -f `krad_radio $station_name logname` 2>&1 >/dev/null &

sleep 1
krad_radio $station_name res $width $height

sleep 1
krad_radio $station_name input XMMS2
sleep 1
krad_radio $station_name output Main
sleep 1
krad_radio $station_name xmms2 XMMS2 tcp://:9669

sleep 1
krad_radio $station_name webon $HTTP_PORT $WEBSOCKET_PORT

sleep 1 
jack_connect XMMS2:out_1 ${station_name}:XMMS2_Left
jack_connect XMMS2:out_2 ${station_name}:XMMS2_Right

sleep 1 
krad_radio $station_name plug ${station_name}:XMMS2_Left XMMS2:out_1
krad_radio $station_name plug ${station_name}:XMMS2_Right XMMS2:out_2 

sleep 1 
krad_radio $station_name transmitter_on 3031

sleep 1
#sleep 3.0
#krad_radio $station_name transmitav transmitter 3031 /test.ogv nopass 
krad_radio $station_name transmit av transmitter 3031 /test1.webm nopass "vp8 vorbis" $width $height $bitrate
#krad_radio $station_name transmit video transmitter 3032 /moviedemo.ogv nopass null ${width} ${height} ${bitrate}
#krad_radio $station_name record av ~/Videos/six600110.ogv "theora vorbis" $width $height $bitrate

#station.cmd("transmit video #{server1} /slideshow.webm secretkode null #{width} #{height} #{bitrate}")
#station.cmd("record video \"#{File.expand_path('~')}/Videos/textdemo.webm\" null #{width} #{height} #{bitrate}")

sleep 8
krad_radio $station_name addsprite "/home/dsheeler/Pictures/anim0316-1_e0_frames_6.png" 128 128
krad_radio $station_name addsprite "/home/dsheeler/Pictures/anim0316-1_e0_frames_6.png" 1152 128
sleep 8

krad_radio $station_name addsprite "/home/dsheeler/Pictures/anim0316-1_e0_frames_6.png" 256 256
krad_radio $station_name addsprite "/home/dsheeler/Pictures/anim0316-1_e0_frames_6.png" 1024 256

sleep 8
krad_radio $station_name addsprite "/home/dsheeler/Pictures/anim0316-1_e0_frames_6.png" 384 384
krad_radio $station_name addsprite "/home/dsheeler/Pictures/anim0316-1_e0_frames_6.png" 896 384


while true; do 

sleep 1
krad_radio $station_name addtext "six600110" 125 1035 3 174 
krad_radio $station_name settext 0 125 255 15 174 0.5 0 255 11 2

sleep 1
krad_radio $station_name addtext "KRAD RADIO" 25 -315 2 190 1 0 0 125 125 "LiberationSans"
krad_radio $station_name settext 1 25 455 10 190 1 0 255 11 2

sleep 8
krad_radio $station_name settext 0 255 200 15 2 1 555  
krad_radio $station_name settext 1 255 200 15 2 1 -555 

sleep 3
krad_radio $station_name rmtext 0
krad_radio $station_name rmtext 1 


#station.set_sprite(0, 333, 155, 5, 4.5, 1, 25)
#sleep 1
#station.set_sprite(0, 33, 65, 6, 4.5, 1, 325)
#sleep 2
#station.set_sprite(0, 533, 235, 7, 4.5, 1, 5)
#sleep 3
#station.throw_sprite(0)
#sleep 2
#station.rm_sprite(0)
sleep 1
krad_radio $station_name addtext "Thanks for watching" 135 655 5 64 1 0 125 155 155 "DroidSans"
sleep 2
krad_radio $station_name settext 0 1535 -155 15 4 0 -50
sleep 2
krad_radio $station_name addtext "See you next time, space cowboy" 135 655 5 64 1 0 125 155 25 "Georgia"
krad_radio $station_name settext 1 135 655 5 64 1 0 25 255 225
sleep 1
krad_radio $station_name settext 1 135 655 25 64 1 0 255 1 1
sleep 1
krad_radio $station_name settext 1 135 655 25 64 1 0 255 1 1
krad_radio $station_name rmtext 0
sleep 1
krad_radio $station_name settext 1 135 655 25 4 0 0 255 1 1
sleep 1
krad_radio $station_name rmtext 1

#sleep 60
#krad_radio $station_name transmitter_off
#krad_radio $station_name destroy
sleep 1
krad_radio $station_name addtext "KRAD RADIO" 25 1035 5 174 
krad_radio $station_name settext 0 25 255 15 174 1 0 255 11 2

sleep 1

krad_radio $station_name addtext "six600110 is happy" 25 -315 5 94 1 0 0 125 125 "LiberationSans"
krad_radio $station_name settext 1 25 455 10 90 1 0 0 125 125

sleep 8

krad_radio $station_name settext 0 -1535 1555 15 4 0 -50
krad_radio $station_name settext 1 1535 1555 15 4 0 -50

sleep 2
krad_radio $station_name rmtext 1
krad_radio $station_name rmtext 0

done
#perl -e 'while (1) {system "krad_radio moviedemo settext 1 55 1000 40 10 0.5 0 0 125 125"; sleep 1; system "krad_radio moviedemo settext 1 55 -100 40 10 0.5 0 0 125 125"; sleep 1; }'&

#perl -e 'while (1) {system "krad_radio moviedemo settext 0 1400 255 10 174"; sleep 1; system "krad_radio moviedemo settext 0 -1900 255 10 174"; sleep 1; }'&
