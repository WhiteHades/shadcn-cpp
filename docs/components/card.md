# Card

Status: draft. Native build, interaction, visual, animation and accessibility checks are unverified.

Header, description, action, content and footer layouts. Shared card colours, borders and spacing.

```cpp
// In the shadcn namespace; parent is a live QWidget*.
Card card(parent);
card.setTitle("Modern C++");
card.setDescription("Continue the current lesson.");
card.content().addWidget(new Button("Continue", &card));
```

Content, footer and action return borrowed Qt layouts. The card owns the layout hosts and their child widgets.

## Known differences

* CSS `shadow-sm` is absent.
* Slots are layout accessors rather than eight separate upstream slot classes.
* CSS container queries and arbitrary class overrides are not implemented.

Shared font, gamut mapping and interpolation differences also apply. See the [source records](https://github.com/WhiteHades/shadcn-cpp/blob/main/upstream/manifest.json).

## Source

[apps/v4/registry/new-york-v4/ui/card.tsx](https://github.com/shadcn-ui/ui/blob/98a1fe67b439324ddc857f47fbdce056600a4329/apps/v4/registry/new-york-v4/ui/card.tsx)

Git blob `6aebd4cf612dfa24d40ffdf357c6f6363e144d6a`. The source wrapper was read; that does not establish complete dependency behaviour.
