var _private = 10;
function _func() {
  this._field = _private;
}
var a = new _func();
print(a._field);
