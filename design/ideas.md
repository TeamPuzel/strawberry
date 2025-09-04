# General ideas for the Strawberry Programming Language

```cpp
[[clang::always_inline]] [[gnu::hot]] [[gnu::const]] auto solid_at(i32 x, i32 y) const -> bool {
    auto const& tile = solid_tile(x / 16, y / 16);
    return height_tiles
        | draw::grid(16, 16)
        | draw::tile(tile.x, tile.y)
        | draw::apply_if(tile.mirror_x, draw::mirror_x())
        | draw::apply_if(tile.mirror_y, draw::mirror_y())
        | draw::get(x % 16, y % 16)
        | draw::eq(draw::color::WHITE);
}
```

```
// Cool method thing.
fun solid_at(&self, x, y: Int32) -> Bool {
    let ref tile = self | tile at x: x / 16 y: y / 16]

    self.height_tiles
        .grid_of(width: 16, height: 16)
        .tile_at(x: tile.x, y: tile.y)
        .apply(.mirror_x, if: tile.mirror_x)
        .apply(.mirror_y, if: tile.mirror_y)
        .get(x: x mod 16, y mod 16)
        .eq(with: white)
}

when x == 1 then 1 else 0

when x {
    0 -> 1
    _ -> 0
}
```
