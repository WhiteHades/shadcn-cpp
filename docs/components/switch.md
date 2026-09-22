# Switch

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Default and small tracks, keyboard and pointer activation. Animated thumb, right to left layout and reduced motion.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Switch autoplay(parent);
autoplay.setAccessibleName("Auto play");
autoplay.setSwitchSize(SwitchSize::Sm);
autoplay.setChecked(true);
```

Use inherited toggled for state changes. The public interface supports two states. Inherited QCheckBox APIs are still available and can request states outside that intended contract.

## Known differences

* Qt exposes inherited checkbox accessibility semantics, not a reviewed switch role.
* CSS shadows are absent.
* Radix package implementation and platform assistive technology parity remain unreviewed.

Shared font, gamut mapping and interpolation differences also apply. See the [source records](https://github.com/WhiteHades/shadcn-cpp/blob/main/upstream/manifest.json).

## Source

[apps/v4/registry/new-york-v4/ui/switch.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/switch.tsx)

Git blob `c45932afe4e484c900aa916e81d0e0d67137bedc`. The source wrapper was read; that does not establish complete dependency behaviour.
