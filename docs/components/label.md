# Label

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Qt buddy association, click-to-focus, click-to-toggle and disabled buddy tracking.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Label label("Course name", parent);
label.setBuddy(&input);
```

The buddy is observed through QPointer. A label click focuses it, and activates a QAbstractButton buddy.

## Known differences

- Grouped disabled opacity, selection prevention and mnemonic behaviour need reference checks.

Shared font, gamut-mapping and interpolation differences also apply. See the [parity process](../parity.md).

## Source

[apps/v4/registry/new-york-v4/ui/label.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/label.tsx)

Git blob `5aff7469c384001a3f1b49cc5cf8871b10f10386`. The source wrapper was read; that does not establish complete dependency behaviour.
