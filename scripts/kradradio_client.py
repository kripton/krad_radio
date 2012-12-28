from time import sleep
import shlex
import subprocess
KRAD = "krad_radio"

class Kradradio:
  def __init__(self, sysname):
    self.sysname = sysname

  def cmd(self, cmd):
  	fullcmd = KRAD+" "+self.sysname+" "+cmd
  	print(fullcmd)
  	subprocess.call(shlex.split(fullcmd))
  	sleep(0.5)
