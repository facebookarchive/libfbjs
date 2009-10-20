function outer() {
  var a = 10;
  function Y() {
    return 10;
  }
  eval('print(a)');
  return a;
}
