// Created by Lua (TeamPuzel) on September 20th 2025.
// Copyright (c) 2025 All rights reserved.
#ifndef SIR_H
#define SIR_H

typedef struct SirCategory {

} SirCategory;

typedef struct SirEnumeration {

} SirEnum;

typedef struct SirTuple {

} SirTuple;

typedef struct SirStructure {

} SirStruct;

typedef struct SirFunction {

} SirFunction;

typedef struct SirClass {

} SirClass;

typedef struct SirItem SirItem;
struct SirItem {
    enum SirItemTag {
        SirItem_Category,
        SirItem_Enumaration,
        SirItem_Tuple,
        SirItem_Structure,
        SirItem_Function,
        SirItem_Class
    } tag;
};

// A single source unit of a Strawberry module.
typedef struct SirUnit {

} SirUnit;

// A single Strawberry module.
typedef struct SirModule {

} SirModule;

// A collection of modules and configuration representing the entire program enviroment ready for
// all kinds of transformation, evaluation and lowering passes.
typedef struct SirProgram {

} SirProgram;

#endif
