// Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the LICENSE
// file in the root directory of this source tree.
//
// RUN: %hermes -O -target=HBC %s | %FileCheck --match-full-lines %s
// RUN: %hermes -O -target=HBC -emit-binary -out %t.hbc %s && %hermes %t.hbc | %FileCheck --match-full-lines %s

try {
  undefined.length;
} catch (e) {
  print(e);
}
// CHECK: TypeError: Cannot read property 'length' of undefined

try {
  null.toString();
} catch (e) {
  print(e);
}
// CHECK: TypeError: Cannot read property 'toString' of null

try {
  undefined.length = 5;
} catch (e) {
  print(e);
}
// CHECK: TypeError: Cannot set property 'length' of undefined

try {
  delete null.length;
} catch (e) {
  print(e);
}
// CHECK: TypeError: Cannot delete property 'length' of null

