# Checkbox

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Native checked, unchecked and mixed states. Pointer, keyboard, disabled, invalid and focus handling.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Checkbox complete("Mark complete", parent);
complete.setTristate(true);
complete.setCheckState(Qt::PartiallyChecked);
```

Use inherited state signals. The mixed state dash is a documented native difference from the inspected indicator.

## Known differences

* The mixed state dash is a deliberate native difference, not an indicator identical to the upstream one.
* The check mark is drawn locally, not the upstream Lucide SVG.
* CSS shadows and `transition-shadow` are absent.

Shared font, gamut mapping and interpolation differences also apply. See the [source records](https://github.com/WhiteHades/shadcn-cpp/blob/main/upstream/manifest.json).

## Source

[apps/v4/registry/new-york-v4/ui/checkbox.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/checkbox.tsx)

Git blob `9aaa1baace43e42aba6ee7c3b04adb8ff7e99ea2`. The source wrapper was read; that does not establish complete dependency behaviour.
