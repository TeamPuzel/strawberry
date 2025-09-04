// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#ifndef NUMBER_H
#define NUMBER_H
// Strawberry requires the ability to operate on arbitrary precision numbers, primarily for use
// in compile time evaluation. The compile time capabilities are determined by the host, not the target.
//
// Demoting arbitrary precision numbers to runtime representation is possible but raises a compile error
// if the target can't express the required size.

// Arbtitrarily sized two's complement integer type.
typedef struct Integer {

} Integer;

// Arbtitrarily sized binary coded decimal type.
typedef struct Decimal {

} Decimal;

#endif
