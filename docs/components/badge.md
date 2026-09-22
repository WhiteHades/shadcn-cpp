# Badge

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Six static text variants.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Badge badge("In progress", parent);
badge.setVariant(Variant::Secondary);
```

This draft is a static text label. It has no link target or icon API.

## Known differences

* Link activation, icons, asChild composition, invalid and focus states are not implemented.

Shared font, gamut mapping and interpolation differences also apply. See the [source records](https://github.com/WhiteHades/shadcn-cpp/blob/main/upstream/manifest.json).

## Source

[apps/v4/registry/new-york-v4/ui/badge.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/badge.tsx)

Git blob `1a94cc539326e26227271a0b3599be99e0e55fb6`. The source wrapper was read; that does not establish complete dependency behaviour.
