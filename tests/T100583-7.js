  function g() {
    var i = 0;
    var f = function fib(n) {
      if (n == 0) { return 0; }
      if (n == 1) { return 1; }
      if (n == i) { return i; }
      return fib(n - 1) + fib(n - 2);
    }
    return f(5);
  }
  print(g());
