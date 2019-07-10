// Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the LICENSE
// file in the root directory of this source tree.
//
// RUN: (! %hermes -commonjs %s 2>&1 ) | %FileCheck %s

var = 3;

// CHECK: {{.*}}cjs-error.js:8:5: error: 'identifier' expected in declaration
// CHECK-NEXT: var = 3;
// CHECK-NEXT: ~~~~^
