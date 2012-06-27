#!/usr/bin/env ruby

rootdir = "~/kode/krad_radio"

require "#{rootdir}/scripts/playlist.rb"
require "#{rootdir}/scripts/bug_stream.rb"

station = "ktv1"
stacmd = "krad_radio #{station}"
playlist_dir = "/data2/youtube"

server1 = "phobos.kradradio.com 8088"
server2 = "phobos.kradradio.com 8080"

`#{stacmd} bug 470 404 ~/Pictures/kr3.png`
sleep 0.2
`#{stacmd} transmit audiovideo #{server1} /ktv.webm secretkode`
sleep 0.2

sleep 0.2
`#{stacmd} transmit audio #{server2} /ktv.opus secretkode opus`


t1 = Thread.new { bug_stream (station) }

#play_dir (playlist_dir)
