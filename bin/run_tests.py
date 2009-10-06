#!/usr/bin/env python

import os
from os.path import join, dirname, abspath, basename, isdir, exists
import signal
import subprocess
import sys
import tempfile
import time

VERBOSE = False;

def RunTest(jsshell, test):
  if VERBOSE: print "running test %s ..." % test;
  # Run js shell and get normal output
  (fd_out, outname) = tempfile.mkstemp();
  (fd_err, errname) = tempfile.mkstemp();

  cmd = "%s %s" % (jsshell, test);

  (process, exit_code, timed_out) = RunProcess(
    timeout = 1,
    args = cmd,
    stdout = fd_out,
    stderr = fd_err);
  os.close(fd_out);
  os.close(fd_err);

  output = file(outname).read();
  return output;

def Jsxminify(jsxmin, test):
  if VERBOSE: print "jsxmin %s ..." % test;
  # Run js shell and get normal output
  (fd_out, outname) = tempfile.mkstemp();
  (fd_err, errname) = tempfile.mkstemp();

  (process, exit_code, timed_out) = RunProcess(
    timeout = 1,
    args = jsxmin,
    stdin = file(test),
    stdout = fd_out,
    stderr = fd_err);
  os.close(fd_out);
  os.close(fd_err);

  return outname;

MAX_SLEEP_TIME = 0.1;
INITIAL_SLEEP_TIME = 0.0001;
SLEEP_TIME_FACTOR = 1.25;

def RunProcess(timeout, args, **rest):
  popen_args = args;
  process = subprocess.Popen(args = popen_args, shell=True, **rest);

  if timeout is None:
    end_time = None;
  else:
    end_time = time.time() + timeout;

  timed_out = False;
  exit_code = None;
  sleep_time = INITIAL_SLEEP_TIME;
  
  while exit_code is None:
    if (not end_time is None) and (time.time() >= end_time):
      os.kill(process.pid, signal.SIGTERM)
      exit_code = process.wait();
      timed_out = True;
    else:
      exit_code = process.poll();
      time.sleep(sleep_time);
      sleep_time = sleep_time * SLEEP_TIME_FACTOR;
      if (sleep_time > MAX_SLEEP_TIME):
        sleep_time = MAX_SLEEP_TIME;

  return (process, exit_code, timed_out);

def Main():
  # Get a list of JS files in tests directory.
  workspace = abspath(join(dirname(sys.argv[0]), '..'));
  shell = join(workspace, 'bin', 'js');
  jsxmin = join(workspace, 'jsxmin');
  repository = join(workspace, 'tests');

  names = os.listdir(repository);
  for name in names:
    if (name.endswith('.js')) :
      full_name = join(repository, name);
      expected = RunTest(shell, full_name);
      temp_js = Jsxminify(jsxmin, full_name);
      output = RunTest(shell, temp_js);

      if (expected == output):
        print "PASS : %s" % full_name;
      else:
        print "FAIL : %s" % full_name;
        print "  Expect %s" % expected;
        print "  Got %s\n" % output;
 

if __name__ == '__main__':
  sys.exit(Main());
