// Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the LICENSE
// file in the root directory of this source tree.
//
// RUN: %hermes -dump-ast --pretty-json %s | %FileCheck %s --match-full-lines

// CHECK-LABEL: {
// CHECK-NEXT:   "type": "Program",
// CHECK-NEXT:   "body": [

({['x']: 3});
// CHECK-NEXT:     {
// CHECK-NEXT:       "type": "ExpressionStatement",
// CHECK-NEXT:       "expression": {
// CHECK-NEXT:         "type": "ObjectExpression",
// CHECK-NEXT:         "properties": [
// CHECK-NEXT:           {
// CHECK-NEXT:             "type": "Property",
// CHECK-NEXT:             "key": {
// CHECK-NEXT:               "type": "StringLiteral",
// CHECK-NEXT:               "value": "x"
// CHECK-NEXT:             },
// CHECK-NEXT:             "value": {
// CHECK-NEXT:               "type": "NumericLiteral",
// CHECK-NEXT:               "value": 3
// CHECK-NEXT:             },
// CHECK-NEXT:             "kind": "init",
// CHECK-NEXT:             "computed": true
// CHECK-NEXT:           }
// CHECK-NEXT:         ]
// CHECK-NEXT:       },
// CHECK-NEXT:       "directive": null
// CHECK-NEXT:     },

var {['x']: z} = {x: 3};
// CHECK-NEXT:     {
// CHECK-NEXT:       "type": "VariableDeclaration",
// CHECK-NEXT:       "kind": "var",
// CHECK-NEXT:       "declarations": [
// CHECK-NEXT:         {
// CHECK-NEXT:           "type": "VariableDeclarator",
// CHECK-NEXT:           "init": {
// CHECK-NEXT:             "type": "ObjectExpression",
// CHECK-NEXT:             "properties": [
// CHECK-NEXT:               {
// CHECK-NEXT:                 "type": "Property",
// CHECK-NEXT:                 "key": {
// CHECK-NEXT:                   "type": "Identifier",
// CHECK-NEXT:                   "name": "x",
// CHECK-NEXT:                   "typeAnnotation": null
// CHECK-NEXT:                 },
// CHECK-NEXT:                 "value": {
// CHECK-NEXT:                   "type": "NumericLiteral",
// CHECK-NEXT:                   "value": 3
// CHECK-NEXT:                 },
// CHECK-NEXT:                 "kind": "init",
// CHECK-NEXT:                 "computed": false
// CHECK-NEXT:               }
// CHECK-NEXT:             ]
// CHECK-NEXT:           },
// CHECK-NEXT:           "id": {
// CHECK-NEXT:             "type": "ObjectPattern",
// CHECK-NEXT:             "properties": [
// CHECK-NEXT:               {
// CHECK-NEXT:                 "type": "Property",
// CHECK-NEXT:                 "key": {
// CHECK-NEXT:                   "type": "StringLiteral",
// CHECK-NEXT:                   "value": "x"
// CHECK-NEXT:                 },
// CHECK-NEXT:                 "value": {
// CHECK-NEXT:                   "type": "Identifier",
// CHECK-NEXT:                   "name": "z",
// CHECK-NEXT:                   "typeAnnotation": null
// CHECK-NEXT:                 },
// CHECK-NEXT:                 "kind": "init",
// CHECK-NEXT:                 "computed": true
// CHECK-NEXT:               }
// CHECK-NEXT:             ]
// CHECK-NEXT:           }
// CHECK-NEXT:         }
// CHECK-NEXT:       ]
// CHECK-NEXT:     },

({['x']: z} = {y: 3});
// CHECK-NEXT:     {
// CHECK-NEXT:       "type": "ExpressionStatement",
// CHECK-NEXT:       "expression": {
// CHECK-NEXT:         "type": "AssignmentExpression",
// CHECK-NEXT:         "operator": "=",
// CHECK-NEXT:         "left": {
// CHECK-NEXT:           "type": "ObjectPattern",
// CHECK-NEXT:           "properties": [
// CHECK-NEXT:             {
// CHECK-NEXT:               "type": "Property",
// CHECK-NEXT:               "key": {
// CHECK-NEXT:                 "type": "StringLiteral",
// CHECK-NEXT:                 "value": "x"
// CHECK-NEXT:               },
// CHECK-NEXT:               "value": {
// CHECK-NEXT:                 "type": "Identifier",
// CHECK-NEXT:                 "name": "z",
// CHECK-NEXT:                 "typeAnnotation": null
// CHECK-NEXT:               },
// CHECK-NEXT:               "kind": "init",
// CHECK-NEXT:               "computed": true
// CHECK-NEXT:             }
// CHECK-NEXT:           ]
// CHECK-NEXT:         },
// CHECK-NEXT:         "right": {
// CHECK-NEXT:           "type": "ObjectExpression",
// CHECK-NEXT:           "properties": [
// CHECK-NEXT:             {
// CHECK-NEXT:               "type": "Property",
// CHECK-NEXT:               "key": {
// CHECK-NEXT:                 "type": "Identifier",
// CHECK-NEXT:                 "name": "y",
// CHECK-NEXT:                 "typeAnnotation": null
// CHECK-NEXT:               },
// CHECK-NEXT:               "value": {
// CHECK-NEXT:                 "type": "NumericLiteral",
// CHECK-NEXT:                 "value": 3
// CHECK-NEXT:               },
// CHECK-NEXT:               "kind": "init",
// CHECK-NEXT:               "computed": false
// CHECK-NEXT:             }
// CHECK-NEXT:           ]
// CHECK-NEXT:         }
// CHECK-NEXT:       },
// CHECK-NEXT:       "directive": null
// CHECK-NEXT:     },

({
  get [foo()]() {},
  set [foo()](val) {},
});
// CHECK-NEXT:     {
// CHECK-NEXT:       "type": "ExpressionStatement",
// CHECK-NEXT:       "expression": {
// CHECK-NEXT:         "type": "ObjectExpression",
// CHECK-NEXT:         "properties": [
// CHECK-NEXT:           {
// CHECK-NEXT:             "type": "Property",
// CHECK-NEXT:             "key": {
// CHECK-NEXT:               "type": "CallExpression",
// CHECK-NEXT:               "callee": {
// CHECK-NEXT:                 "type": "Identifier",
// CHECK-NEXT:                 "name": "foo",
// CHECK-NEXT:                 "typeAnnotation": null
// CHECK-NEXT:               },
// CHECK-NEXT:               "arguments": []
// CHECK-NEXT:             },
// CHECK-NEXT:             "value": {
// CHECK-NEXT:               "type": "FunctionExpression",
// CHECK-NEXT:               "id": null,
// CHECK-NEXT:               "params": [],
// CHECK-NEXT:               "body": {
// CHECK-NEXT:                 "type": "BlockStatement",
// CHECK-NEXT:                 "body": []
// CHECK-NEXT:               },
// CHECK-NEXT:               "generator": false
// CHECK-NEXT:             },
// CHECK-NEXT:             "kind": "get",
// CHECK-NEXT:             "computed": true
// CHECK-NEXT:           },
// CHECK-NEXT:           {
// CHECK-NEXT:             "type": "Property",
// CHECK-NEXT:             "key": {
// CHECK-NEXT:               "type": "CallExpression",
// CHECK-NEXT:               "callee": {
// CHECK-NEXT:                 "type": "Identifier",
// CHECK-NEXT:                 "name": "foo",
// CHECK-NEXT:                 "typeAnnotation": null
// CHECK-NEXT:               },
// CHECK-NEXT:               "arguments": []
// CHECK-NEXT:             },
// CHECK-NEXT:             "value": {
// CHECK-NEXT:               "type": "FunctionExpression",
// CHECK-NEXT:               "id": null,
// CHECK-NEXT:               "params": [
// CHECK-NEXT:                 {
// CHECK-NEXT:                   "type": "Identifier",
// CHECK-NEXT:                   "name": "val",
// CHECK-NEXT:                   "typeAnnotation": null
// CHECK-NEXT:                 }
// CHECK-NEXT:               ],
// CHECK-NEXT:               "body": {
// CHECK-NEXT:                 "type": "BlockStatement",
// CHECK-NEXT:                 "body": []
// CHECK-NEXT:               },
// CHECK-NEXT:               "generator": false
// CHECK-NEXT:             },
// CHECK-NEXT:             "kind": "set",
// CHECK-NEXT:             "computed": true
// CHECK-NEXT:           }
// CHECK-NEXT:         ]
// CHECK-NEXT:       },
// CHECK-NEXT:       "directive": null
// CHECK-NEXT:     }

// CHECK-NEXT:   ]
// CHECK-NEXT: }
