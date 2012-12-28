#!/usr/bin/env python
# Hello Krad World Video in Python
from time import sleep
from kradradio_client import Kradradio

# Station name
station_name = "hellokrad"
station = Kradradio(station_name)
 
# Video resolution and frame rate
width = 640
height = 360
fps = 30
 
# Video filename
recording = "/home/oneman/hello_krad_world.webm"
 
# Station startup
station.cmd("launch")

# Setting resolution
station.cmd("res "+str(width)+" "+str(height))
 
# Setting framerate
station.cmd("fps "+str(fps))

# Add Text
station.cmd("addtext 'Hello Krad World' 120 150 0 4 40 0 0 244 2 2 Helvetica")

# Start Recording
station.cmd("record video "+recording)

# Show Text
station.cmd("settext 0 120 150 0 4 40 1 0")

# Wait
sleep(3)
 
# Fade Text
station.cmd("settext 0 120 150 0 4 40 0 0")

# Wait
sleep(3)
 
# Shutdown
station.cmd("kill")
