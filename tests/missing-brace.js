function foo(a, b) {
  if (a) {
    for (var i = 0; i < 1; i++) {
      if (!a)
        break;
      else if (b)
        print('hello');
    }
  } else {
    print('ok');
  }
}

foo(true, false);
