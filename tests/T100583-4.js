function a(i) {
  if (i > 10) {
    return i;
  }
  return a(i+1);
}
print(a(3));
