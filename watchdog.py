#!/usr/bin/env

'''Watchdog script to keep a krad_radio instance running'''

import subprocess
import time

class Daemon:
  def __init__(self):
    self.cmd = 'krad_radio_daemon'
    self.station = 'test'
    self.config = ['krad_radio', self.station, 'info']
    self.running = False

  def log(self, msg):
    timestamp = time.strftime('%Y-%M-%d %H:%M:%S', time.gmtime())
    print '--- %s - %s ---\n' % (timestamp, msg)

  def run(self, cmd=None):
    if not cmd:
      cmd = self.cmd
    self.running = True
    while self.running:
      self.log('launching %s' % cmd)
      proc = subprocess.Popen([cmd, self.station])
      time.sleep(0.5)
      self.log('configuring')
      ret = subprocess.call(self.config)
      self.log('configuration exited with return code ' + str(ret))
      ret = proc.wait();
      self.log('exited with return code ' + str(ret))

if __name__ == '__main__':
  job = Daemon();
  #job.station = 'test2'
  #job.config = './capture.sh'
  job.run()
