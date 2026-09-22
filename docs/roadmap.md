# Roadmap

The first priority is verifying the existing native code. Adding more unchecked components would make that review harder. Version remains 0.1.0 throughout these increments.

| Step | Deliverable | Gate |
| --- | --- | --- |
| 1 | Compile the ten drafts, gallery and installed widget consumer on Linux, Windows and macOS. | Native tests and supported sanitizer jobs pass. |
| 2 | Build browser references and compare every draft state. Fix shadows, font/colour differences, incomplete states and accessible roles. | Component evidence supports the claimed profile. |
| 3 | Add layout and form foundations, including Textarea, Toggle, Radio Group, Slider, Tabs, Accordion and Field. | Source-specific interactions and composition tests pass. |
| 4 | Add popup behaviour and Dialog, Alert Dialog, Popover, Tooltip, menus, Sheet and Drawer. | Focus, nesting, dismissal, screen-edge and reduced-motion checks pass. |
| 5 | Add Combobox, Command, Select, Calendar, Date Picker and form recipes. | Keyboard, input-method, validation and locale checks pass. |
| 6 | Add model/view Table and Data Table, navigation, Sidebar, notifications and remaining catalogue controls. | Large-data and application-composition tests pass. |
| 7 | Add Chart, Carousel, message components, blocks, alternate profiles and theme presets. | Each profile has its own source and evidence. |
| 8 | Finish distribution and documentation, including reviewed dependency locks and an honest platform matrix. | A fresh machine builds and uses the published snapshot. |

The [catalogue](components/index.md) names every registry entry in the pinned initial profile. Recipes and other profiles remain separately tracked because they are not interchangeable implementations of that profile. Run the upstream inventory before each new batch to find files added or removed upstream.

The [issue directory](issues/01-native-verification.md) contains the first dependency-ordered work items. Split a batch into one-component changes before implementation. Each change must include its tests, documentation, source reference and remaining differences.

A completed step does not authorise a version bump. During the version freeze, exact commits identify snapshots and API changes must appear in the changelog.
