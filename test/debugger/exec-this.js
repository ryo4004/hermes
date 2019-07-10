// Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the LICENSE
// file in the root directory of this source tree.
//
// RUN: %hdb %s < %s.debug | %FileCheck --match-full-lines %s
// REQUIRES: debugger

function foo() {
  function bar() {
    function baz() {
      debugger;
    }
    baz.call(3);
  }
  bar.call('asdf');
}

foo.call(10);

// CHECK: Break on 'debugger' statement in baz: {{.*}}:12:7
// CHECK-NEXT: 3
// CHECK-NEXT: 10
// CHECK-NEXT: Stepped to bar: {{.*}}:14:13
// CHECK-NEXT: asdf
// CHECK-NEXT: asdfasdf
// CHECK-NEXT: Stepped to foo: {{.*}}:16:11
// CHECK-NEXT: 10
// CHECK-NEXT: 13
// CHECK-NEXT: Continuing execution
