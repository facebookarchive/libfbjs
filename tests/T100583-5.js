function a() {
  var a = 3;
  function b(i) {
    var b = 10;
    return a;
  }
  return b(5);
}
print(a());
