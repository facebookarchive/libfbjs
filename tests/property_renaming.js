function Foo() {
  this.a = 0;  // 'a' should not be reused.
  this._prop1 = 1;
  this["_prop2"] =2;
  this["3"] = 3;
  this["_should not rename"] = 4;
}

var x = new Foo();
print(x.a);
print(x._prop1);
print(x['_prop1']);
print(x._prop2);
print(x['_prop2']);
print(x['3']);
print(x['_should not rename']);


var y = {
 _field1 : 1,
 "_field2" : 2,
 "3" : 3,
 "_should not rename" : 4,
 "void" : 5
}

print(y._field1);
print(y['_field1']);
print(y._field2);
print(y['_field2']);
print(y['3']);
print(y['_should not rename']);
print(y['void']);
