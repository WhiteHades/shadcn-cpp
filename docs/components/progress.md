# Progress

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Native progress state, animated determinate fill and empty indeterminate track. Right-to-left and inverted appearance.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Progress progress(parent);
progress.setRange(0, 100);
progress.setValue(39);
```

This draft paints horizontal progress only; inherited vertical orientation and text-format options are not supported by its painter. Values follow QProgressBar integer semantics. A zero-to-zero range is indeterminate and draws an empty track, matching the inspected wrapper rather than a busy stripe.

## Known differences

- Qt values are integers; upstream accepts numbers.
- Range and accessibility differences need Radix and native checks.

Shared font, gamut-mapping and interpolation differences also apply. See the [parity process](../parity.md).

## Source

[apps/v4/registry/new-york-v4/ui/progress.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/progress.tsx)

Git blob `ac48e2e5e25d507fcc68df5f95717b8a3df71867`. The source wrapper was read; that does not establish complete dependency behaviour.
