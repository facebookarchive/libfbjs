function a() {
  function b() {
    return 1;
  }
  return function() {
    return b();
  }
}
print(a()());
