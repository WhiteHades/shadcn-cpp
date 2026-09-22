# Input

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

QLineEdit text editing, password mode, placeholder and validation description. Border and focus rendering.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Input input(parent);
input.setPlaceholderText("Course name");
input.setError("A course name is required.");
```

An empty error clears the invalid state and accessible description. QLineEdit methods supply native editing behaviour.

## Known differences

* Colour and shadow transitions are absent.
* HTML file, date and numeric input types are not implemented.
* Disabled placeholder and selection opacity need reference checks.

Shared font, gamut mapping and interpolation differences also apply. See the [source records](https://github.com/WhiteHades/shadcn-cpp/blob/main/upstream/manifest.json).

## Source

[apps/v4/registry/new-york-v4/ui/input.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/input.tsx)

Git blob `ddb9b315e34245addded54b1848bb32c80c418cc`. The source wrapper was read; that does not establish complete dependency behaviour.
