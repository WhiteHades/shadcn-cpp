# Components

The initial source profile contains 62 registry entries. Ten have draft C++ implementations. The Linux build and tests pass with Qt 6.11.2; visual and accessibility parity remain under review. A listed class is not a support claim for every upstream option.

| Component | Status | Current scope |
| --- | --- | --- |
| [Button](button.md) | Draft | Six variants and eight sizes. |
| [Card](card.md) | Draft | Header, description, action, content and footer layouts. |
| [Input](input.md) | Draft | QLineEdit text editing, password mode, placeholder and validation description. |
| [Switch](switch.md) | Draft | Default and small tracks, keyboard and pointer activation. |
| [Badge](badge.md) | Draft | Six static text variants. |
| [Progress](progress.md) | Draft | Native progress state, animated determinate fill and empty indeterminate track. |
| [Label](label.md) | Draft | Qt buddy association, focus and toggle actions on click and disabled buddy tracking. |
| [Separator](separator.md) | Draft | Decorative horizontal and vertical lines one logical pixel thick. |
| [Checkbox](checkbox.md) | Draft | Native checked, unchecked and mixed states. |
| [Skeleton](skeleton.md) | Draft | Opacity pulse lasting two seconds with shared accent colour and radius. |

The [source records](https://github.com/WhiteHades/shadcn-cpp/blob/main/upstream/manifest.json) list the reference files and known differences for each component.
