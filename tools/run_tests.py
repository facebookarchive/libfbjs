#!/usr/bin/env python

import os
from os.path import join, dirname, abspath, basename, isdir, exists
import optparse
import signal
import subprocess
import sys
import tempfile
import time

# These are global variables used by functions.
JSSHELL = None
JSXMIN = None
VERBOSE = 0 

TOTAL_PASSED = 0
TOTAL_FAILED = 0

def RunTest(test):
  if VERBOSE >= 2: print 'running test %s ...' % test
  # Run js shell and get normal output
  (fd_out, outname) = tempfile.mkstemp()
  (fd_err, errname) = tempfile.mkstemp()

  cmd = '%s %s' % (JSSHELL, test)

  (process, exit_code, timed_out) = RunProcess(
    timeout = 1,
    args = cmd,
    stdout = fd_out,
    stderr = fd_err)
  os.close(fd_out)
  os.close(fd_err)

  output = file(outname).read()
  return output

def Jsxminify(test):
  if VERBOSE >= 2: print '%s %s ...' % (JSXMIN, test)
  # Run js shell and get normal output
  (fd_out, outname) = tempfile.mkstemp()
  (fd_err, errname) = tempfile.mkstemp()

  (process, exit_code, timed_out) = RunProcess(
    timeout = 1,
    args = JSXMIN,
    stdin = file(test),
    stdout = fd_out,
    stderr = fd_err)
  os.close(fd_out)
  os.close(fd_err)

  return outname

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

def GetOptionParser():
  usage = 'usage: %prog [options] test1 test2_dir ...'
  parser = optparse.OptionParser(usage=usage)

  parser.add_option('--jsshell', action='store', type='string', dest='jsshell',
                    help='JS engine path, e.g., ./tools/js')
  parser.add_option('--jsxmin', action='store', type='string', dest='jsxmin',
                    help='JSXMIN binary path, e.g., ./jsxmin')
  parser.add_option('--jsxminargs', action='store', type='string',
                    dest='jsxminargs',
                    help='arguments passed to jsxmin, e.g, --pretty')
  parser.add_option('--verbose', action='store', type='int', dest='verbose',
                    default=0, help='Verbose level, e.g., 1, 2')
  return parser

def VerifyTest(name):
  global TOTAL_PASSED, TOTAL_FAILED

  expected = RunTest(name)
  temp_js = Jsxminify(name)
  output = RunTest(temp_js)

  if (expected == output):
    TOTAL_PASSED += 1
    if VERBOSE >= 1: print 'PASS : %s' % name
  else:
    TOTAL_FAILED += 1
    print 'FAIL : %s' % name
    print '  Expect %s' % expected
    print '  Got %s\n' % output
 
def VerifyTestsInDir(dirname):
  files = os.listdir(dirname)
  for file in files:
    name = join(dirname, file)
    if isdir(name):
      VerifyTestsInDir(name)
    elif (name.lower().endswith('.js')) :
      VerifyTest(name)

def Main():
  global JSSHELL, JSXMIN, VERBOSE

  parser = GetOptionParser()
  (options, args) = parser.parse_args()

  workspace = abspath(join(dirname(sys.argv[0]), '..'))

  if not options.jsshell:
    JSSHELL = join(workspace, 'tools', 'js')
  else:
    JSSHELL = options.jsshell

  if not exists(JSSHELL):
    print '%s does not exist' % JSSHELL 
    return 1
 
  if not options.jsxmin:
    JSXMIN = join(workspace, 'jsxmin')
  else:
    JSXMIN = options.jsxmin

  if not exists(JSXMIN):
    print '%s does not exist' % JSXMIN 
    return 1

  VERBOSE = options.verbose

  if not args:
    args.append(join(workspace, 'tests'))

  for name in args:
    if isdir(name):
      VerifyTestsInDir(abspath(name))
    elif name.lower().endswith('.js'):
      VerifyTest(abspath(name))
    else:
      print 'Ignore %s' % name

  # print out stats
  total_tests = TOTAL_PASSED + TOTAL_FAILED
  print 'Total %d tests %d failed' % (total_tests, TOTAL_FAILED)

if __name__ == '__main__':
  sys.exit(Main())
