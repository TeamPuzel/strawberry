#ifndef MANGLE_H
#define MANGLE_H

// TODO: Symbol mangling
//
// To compose with existing platform tooling and limitations there needs to be a way to encode function
// signatures which are at a similar level of complexity to Swift.
//
// There will be a prefix.
// Generally $str0_ but _Str0_ should be supported in case a platform can't handle or does not need the dollar sign.
// Hopefully this does not collide with any existing language, in that case it will have to be changed.
// The digit embedded in the mangled name is the mangling version. This is unlikely to be needed and it's possible
// that the stable strawberry release will drop the versioning, but it's better to be safe than sorry.
//
// Components of the mangling are then prefixed with numbers indicating count.
//
// I still need to figure out generic instantiation mangling.
static inline void mangle() {

}

#endif
