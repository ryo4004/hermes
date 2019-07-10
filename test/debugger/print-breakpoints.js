// Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the LICENSE
// file in the root directory of this source tree.
//
// RUN: %hdb %s < %s.debug | %FileCheck --match-full-lines %s
// REQUIRES: debugger

print('print breakpoints');
// CHECK-LABEL: print breakpoints

function foo() {
  print('first');
  print('second');
  print('third');
}

debugger;
foo();

// CHECK-NEXT: Break on 'debugger' statement in global: {{.*}}
// CHECK-NEXT: Set breakpoint 1 at {{.*}}:13:3
// CHECK-NEXT: Set breakpoint 2 at {{.*}}:14:3
// CHECK-NEXT: Set breakpoint 3 at {{.*}}:15:3
// CHECK-NEXT: Disabled breakpoint 1
// CHECK-NEXT: Disabled breakpoint 2
// CHECK-NEXT: 1 D {{.*}}print-breakpoints.js:13:3
// CHECK-NEXT: 2 D {{.*}}print-breakpoints.js:14:3
// CHECK-NEXT: 3 E {{.*}}print-breakpoints.js:15:3
// CHECK-NEXT: Deleted breakpoint 1
// CHECK-NEXT: Deleted breakpoint 3
// CHECK-NEXT: 2 D {{.*}}print-breakpoints.js:14:3
// CHECK-NEXT: Continuing execution
// CHECK-NEXT: first
// CHECK-NEXT: second
// CHECK-NEXT: third
