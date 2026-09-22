# Button

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Six variants and eight sizes. Space and Enter activation, disabled and invalid state, hover transitions.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Button button("Continue", parent);
button.setVariant(Variant::Outline);
button.setButtonSize(ButtonSize::Sm);
```

Use the inherited clicked signal with a receiver context. Button dimensions may grow for larger text to avoid clipping.

## Known differences

* CSS shadows are absent.
* Native font metrics and icon rendering differ.
* React asChild composition has no direct C++ equivalent.

Shared font, gamut mapping and interpolation differences also apply. See the [source records](https://github.com/WhiteHades/shadcn-cpp/blob/main/upstream/manifest.json).

## Source

[apps/v4/registry/new-york-v4/ui/button.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/button.tsx)

Git blob `e3345d985d14c33e3cb9d8c45cf973807326c944`. The source wrapper was read; that does not establish complete dependency behaviour.
