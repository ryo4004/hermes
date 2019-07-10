// Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the LICENSE
// file in the root directory of this source tree.
//
// RUN: %hdb --break-at-start %s < %s.debug | %FileCheck --match-full-lines %s
// REQUIRES: debugger

var obj = {};
obj[Symbol.toPrimitive] = function(hint) {
  if (hint === "number") {
    return 1;
  } else {
    return true;
  }
}

print("negate");
print(-obj);

print("add");
print("hello " + obj);

// CHECK: Break on script load in global: {{.*}}:9:1
// CHECK: Set breakpoint 1 at {{.*}}:11:7
// CHECK: Continuing execution
// CHECK: negate
// CHECK: Break on breakpoint 1 in anonymous: {{.*}}:11:7
// CHECK: Stepped to global: {{.*}}:19:7
// CHECK: Continuing execution
// CHECK: -1
// CHECK: add
// CHECK: Break on breakpoint 1 in anonymous: {{.*}}:11:7
// CHECK: Stepped to global: {{.*}}:22:7
// CHECK: Continuing execution
// CHECK: hello true
