#ifndef BACKEND_H
#define BACKEND_H

// Backend interface ---------------------------------------------------------------------------------------------------

// The runtime interface of a target backend.
typedef struct Backend {
    // TODO(blocking: DECL): The signature should receive a pointer to the resolved program declaration.
    void (*lower)();
} Backend;

#endif
