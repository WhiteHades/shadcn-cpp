# Separator

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Decorative horizontal and vertical one-logical-pixel lines.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Separator line(Qt::Horizontal, parent);
```

The current separator is decorative. Layout policy gives one logical pixel to its narrow dimension.

## Known differences

- Non-decorative accessible separator semantics are not implemented.

Shared font, gamut-mapping and interpolation differences also apply. See the [parity process](../parity.md).

## Source

[apps/v4/registry/new-york-v4/ui/separator.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/separator.tsx)

Git blob `6a553b978c11cf82c93817b2d0dde1db0e555835`. The source wrapper was read; that does not establish complete dependency behaviour.
