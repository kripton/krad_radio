#!/usr/bin/env ruby

station = "radio_test"

stacmd = "krad_radio #{station}"

`#{stacmd} input Music1`
`#{stacmd} input Music2`
`#{stacmd} output Main`
`#{stacmd} xfade Music1 Music2`


`jack_connect XMMS2:out_1 #{station}:Music1_Left`
`jack_connect XMMS2:out_2 #{station}:Music1_Right`

`jack_connect XMMS2-01:out_1 #{station}:Music2_Left`
`jack_connect XMMS2-01:out_2 #{station}:Music2_Right`

sleep 2
`#{stacmd} capture v4l2`
sleep 2
`#{stacmd} transmitav phobos.kradradio.com 8080 /#{station}.webm secretkode`
sleep 2
`#{stacmd} vuon`
sleep 2
`#{stacmd} webon 13000 13001`
sleep 2
`#{stacmd} transmit audio phobos.kradradio.com 8080 /#{station}.opus secretkode opus`
