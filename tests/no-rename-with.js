function foo(o) {
  with (o) {
   var a = 10;
  }
  print(o.a);
}

foo({b:5});
