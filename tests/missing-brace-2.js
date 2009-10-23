function foo(a, b) {
  if (a) {
    if (b) {
      print('b');
    }
  } else {
    print('a');
  }
}

foo(true, false);
