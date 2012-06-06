#!/usr/bin/env

'''Watchdog script to keep a krad_radio instance running'''

import subprocess
import time

class Daemon:
  def __init__(self):
    self.cmd = 'krad_radio_daemon'
    self.station = 'test'
    self.running = False

  def log(self, msg):
    timestamp = time.strftime('%Y-%M-%d %H:%M:%S', time.gmtime())
    print '--- %s - %s ---\n' % (timestamp, msg)

  def run(self, cmd=None):
    if not cmd:
      cmd = self.cmd
    self.running = True
    while self.running:
      self.log('running %s' % cmd)
      ret = subprocess.call([cmd, self.station])
      self.log('finished return ' + str(ret))

if __name__ == '__main__':
  job = Daemon();
  job.run()
