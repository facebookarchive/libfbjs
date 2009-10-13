#!/usr/bin/env python

import os
from os.path import join, dirname, abspath, isfile, isdir, exists
import optparse
import shutil
import signal
import subprocess
import sys
import tempfile
import time

RESTORE = False
JSXMIN = None
JSXMINARGS = None
VERBOSE = 0

def Jsxminify(orig, dest):
  if VERBOSE >= 2: print '%s < %s > %s ...' % (JSXMIN, orig, dest)

  cmd = '%s %s' % (JSXMIN, JSXMINARGS)

  (fd_out, outname) = tempfile.mkstemp()
  (fd_err, errname) = tempfile.mkstemp()
  (process, exit_code, timed_out) = RunProcess(
    timeout = 1,
    args = cmd,
    stdin = file(orig),
    stdout = fd_out,
    stderr = fd_err)
  os.close(fd_out)
  os.close(fd_err)

  error = file(errname).read()
  if error:
    print 'ERROR: %s' % dest
    print error
    return

  # overwrite the dest file
  shutil.copyfile(outname, dest)

MAX_SLEEP_TIME = 0.1
INITIAL_SLEEP_TIME = 0.0001
SLEEP_TIME_FACTOR = 1.25

def RunProcess(timeout, args, **rest):
  popen_args = args
  process = subprocess.Popen(args = popen_args, shell=True, **rest)

  if timeout is None:
    end_time = None
  else:
    end_time = time.time() + timeout

  timed_out = False
  exit_code = None
  sleep_time = INITIAL_SLEEP_TIME
  
  while exit_code is None:
    if (not end_time is None) and (time.time() >= end_time):
      os.kill(process.pid, signal.SIGTERM)
      exit_code = process.wait()
      timed_out = True
    else:
      exit_code = process.poll()
      time.sleep(sleep_time)
      sleep_time = sleep_time * SLEEP_TIME_FACTOR
      if (sleep_time > MAX_SLEEP_TIME):
        sleep_time = MAX_SLEEP_TIME

  return (process, exit_code, timed_out)

def TranslateFile(dir, file):
  orig = join(dir, '.' + file + '.orig')
  dest = join(dir, file)
  if not exists(orig):
    shutil.copyfile(dest, orig)

  Jsxminify(orig, dest)

def RestoreFile(dir, file):
  # constructure the js file name
  orig = join(dir, file)
  start_idx = 1 # '.'
  end_idx = -5 # '.orig'
  dest = join(dir, file[start_idx:end_idx])
  shutil.copyfile(orig, dest)

def ProcessFile(dir, file):
  if RESTORE:
    if file.lower().startswith('.') and file.lower().endswith('.js.orig'):
      RestoreFile(dir, file)
  else:
    if file.lower().endswith('.js') :
      TranslateFile(dir, file)

def ProcessDir(dir):
  files = os.listdir(dir)
  for file in files:
    name = join(dir, file)
    if isdir(name):
      ProcessDir(name)
    elif isfile(name):
      ProcessFile(dir, file)

def GetOptionParser():
  usage = 'usage: %prog [options] test_dir'
  parser = optparse.OptionParser(usage=usage)

  parser.add_option('-r', '--restore', action='store_true', dest='restore',
                    default=False, help='Restore to the original js files')
  parser.add_option('--jsxmin', action='store', type='string', dest='jsxmin',
                    help='JSXMIN binary path, e.g., ./jsxmin')
  parser.add_option('--jsxminargs', action='store', type='string',
                    dest='jsxminargs',
                    help='Arguments passed to jsxmin, e.g., --pretty')
  parser.add_option('--verbose', action='store', type='int', dest='verbose',
                    default=0, help='Verbose level, e.g., 1, 2')
  return parser

def Main():
  global RESTORE, JSXMIN, VERBOSE

  parser = GetOptionParser()
  (options, args) = parser.parse_args()

  workspace = abspath(join(dirname(sys.argv[0]), '..'))

  if not options.jsxmin:
    JSXMIN = join(workspace, 'jsxmin')
  else:
    JSXMIN = options.jsxmin

  if not exists(JSXMIN):
    print '%s does not exist' % JSXMIN 
    return 1

  VERBOSE = options.verbose
  RESTORE = options.restore
  JSXMINARGS = options.jsxminargs

  if not args:
    parser.print_help()
    return 1

  for dir in args:
    if isdir(dir):
      ProcessDir(dir)

if __name__ == '__main__':
  sys.exit(Main())

