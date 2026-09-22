# Skeleton

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Opacity pulse lasting two seconds with shared accent colour and radius. Animation stops while hidden or when reduced motion is selected.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Skeleton loading(parent);
loading.setFixedHeight(20);
```

The placeholder pulses only when visible and full motion is selected. It becomes static under MotionPolicy::Reduced.

## Known differences

* Timing and pixel comparisons between the browser and Qt have not run.

Shared font, gamut mapping and interpolation differences also apply. See the [source records](https://github.com/WhiteHades/shadcn-cpp/blob/main/upstream/manifest.json).

## Source

[apps/v4/registry/new-york-v4/ui/skeleton.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/skeleton.tsx)

Git blob `a1ceb0545be73aa39a336271ecb69ed6aa69a64d`. The source wrapper was read; that does not establish complete dependency behaviour.
