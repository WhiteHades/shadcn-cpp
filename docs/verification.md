# Verification

Recorded on 22 September 2026. Version: 0.1.0.

The Git bundle was restored with both original commits intact. This is a development snapshot, not a tagged release.

## Local publication checks

The restored checkout was tested on Linux with GCC 16.2.1, CMake 4.4.3 and Qt 6.11.2. The label test needed an explicit empty text argument when constructing its checkbox through `make_child`. No public API changed.

| Check | Result |
| --- | --- |
| Git bundle verification and `git fsck --full` | Passed; complete history. |
| `python3 tools/check.py` | Passed for version 0.1.0, 62 registry entries and 10 draft records. |
| Python tooling tests | Eight passed, including C++ reference formatting. |
| Core build with warnings treated as errors | Passed; 1043 assertions. |
| Core AddressSanitizer and UndefinedBehaviorSanitizer tests | Passed. |
| Native library, gallery and tests | Built. Core and widget CTest executables passed; QtTest reported 13 passes including setup and cleanup. [Native output](evidence/native-local.txt). |
| Installed core package and separate consumer | Passed. |
| Native sanitizer tests | Widget assertions passed, but LeakSanitizer failed on platform-library allocations. See below. |
| C++ API extraction and Docusaurus build | Passed with clang-doc 22.1.8 and Node 22.20.0. Generated 16 record pages and a namespace page containing functions and enums. [Build output](evidence/documentation-local.txt). |
| Documentation browser checks | API overview and generated Button page inspected at desktop and mobile widths. Sidebar navigation worked; no page errors or document-wide horizontal overflow. |

Commands used the existing CMake presets with build directories under `.tmp/publication`: `core`, `core-asan`, `dev` and `asan`. Each was configured with `cmake --preset <name> -B .tmp/publication/<name>`, built with `cmake --build`, and tested with `ctest --test-dir ... --output-on-failure`. The core package was installed into `.tmp/publication/install` and consumed by `examples/consumer` through `CMAKE_PREFIX_PATH`.

The first native sanitizer run reported 1,533,255 leaked bytes through GTK/font libraries and the graphics driver. Removing desktop theme selection reduced the report to 183 bytes in four allocations originating in `libnvidia-glcore.so.610.57.04`. The [recorded rerun](evidence/native-sanitizer-local.txt) used:

```sh
env -u QT_QPA_PLATFORMTHEME -u QT_STYLE_OVERRIDE \
  -u XDG_CURRENT_DESKTOP -u DESKTOP_SESSION \
  ctest --test-dir .tmp/publication/asan --output-on-failure
```

Setting `QT_QPA_OFFSCREEN_NO_GLX=1` also left the same 183-byte report. Leak detection was not disabled or suppressed. Native sanitizer clearance remains pending; these results do not establish visual or platform accessibility parity.

### Documentation dependencies

The npm lockfile preserves the five direct dependency pins. All 1,270 resolved packages use `registry.npmjs.org` and have integrity hashes. `mise exec node@22 -- bash tools/build-docs.sh` passed the complete extraction, clean dependency installation and site build. Builds use `npm ci --ignore-scripts`. The documentation build now uses clang-doc and Docusaurus without Doxygen. Removing the original package's `type: module` setting fixed Docusaurus's generated route registry during server rendering; the configuration files retain their `.mjs` extensions.

The first npm audit reported 21 findings, including propagated dependency findings. `serialize-javascript@6.0.2` has [code execution](https://github.com/advisories/GHSA-5c6j-r48x-rmvq) and [denial of service](https://github.com/advisories/GHSA-qj8w-gfj5-8c6v) advisories in the documentation build tools. It is not part of the generated static site's runtime. A compatible lockfile-only update was unavailable; a major-version override has not been validated. The remaining `uuid` advisory concerns APIs that the locked SockJS caller does not use. The build-tool advisories remain unresolved. Disabling install scripts does not fix those advisories.

## Original authoring snapshot

The records below describe the original bundle before the local checks above. The authoring environment had no Qt SDK or documentation dependencies, and its GitHub connector could not create a repository. Those historical limitations do not describe the current checkout.

### Checks that ran

| Check | Result | Evidence |
| --- | --- | --- |
| GCC 14.2 core build with warnings treated as errors | Passed. | [Build and assertions](evidence/core.txt) |
| Qt-free unit executable | Passed, 1043 assertions including a 1001-point timing sweep. | [Output](evidence/core.txt) |
| AddressSanitizer and UndefinedBehaviorSanitizer core build | Passed, with sanitizer recovery disabled. | [Sanitizer run](evidence/core-asan.txt) |
| Installed CMake core package and separate consumer | Passed. | [Consumer run](evidence/installed-consumer.txt) |
| Fresh source archive, core build and installed consumer | Passed without copying previous build outputs. | [Archive smoke test](evidence/archive-smoke.txt) |
| Provenance, path-safety and evidence-gate tools | Seven Python tests passed. | [Tool tests](evidence/tools.txt) |
| Version, catalogue and manifest consistency | Passed for 62 registry entries and 10 draft component records. | [Checker output](evidence/tools.txt) |
| Documentation JavaScript and shell script syntax | Passed. This is not a documentation build or a publishing test. | [Syntax checks](evidence/syntax.txt) |
| Retained upstream MIT notice | Local contents match Git blob `fad4d887a681dd49233e5ed01ee2c7a1513089a0`. | `LICENSES/shadcn-MIT.txt` |

The machine-readable [run record](evidence/results.json) contains commands and exit codes. A successful core sanitizer run does not exercise Qt, widget lifetimes or platform libraries.

### Checks that could not run

| Check | Status |
| --- | --- |
| Native Qt configuration and compilation | Blocked. The environment has no Qt 6 SDK. [CMake error](evidence/native-configure.txt). |
| Native unit tests and gallery rendering | Not run. They require the missing Qt SDK. |
| Native memory-safety checks | Not run. The configured Linux job still needs its first execution. |
| Browser-to-native visual and animation comparisons | Not run. No approved reference capture set exists yet. |
| Screen readers, input methods and desktop compositors | Not run. |
| Original documentation build | Blocked in the authoring environment. The current setup uses Docusaurus and clang-doc. [Historical prerequisite error](evidence/documentation.txt). |
| npm dependency resolution and lockfile | Not run. No fabricated lockfile is included. |
| Full checkout-based upstream audit | Not run. The exact source files and Git blob identifiers were read through GitHub; the audit tool ran against a temporary local fixture repository. |
| GitHub Actions and Pages | Configured only. No remote run or deployment occurred. |
| Public repository creation and push | Not performed. The publishing script requires an authenticated GitHub CLI with write permission on your machine. |

## Source review

The initial wrapper review covered Button, Card, Input, Badge, Label, Checkbox, Switch, Separator, Progress and Skeleton. The reference is shadcn-ui/ui commit `98a1fe67b439324ddc857f47fbdce056600a4329`, New York v4 and the neutral theme. The registry and licence were also read. The manifest records exact paths and hashes.

Manual review found missing Enter activation, incomplete custom-control hit areas and focus-frame issues during parenting. The source now includes corrections and native regression cases. The subsequent Linux tests above exercise those regression cases. Cross-platform verification remains pending. The focus-frame review also read Qt's [v6.8.3 implementation](https://github.com/qt/qtbase/blob/v6.8.3/src/widgets/widgets/qfocusframe.cpp).

Known gaps include shadows, some transitions and states, link-badge composition, vertical progress options, external behaviour dependencies, precise icon/font matching, colour gamut mapping and platform accessibility. The component records list the relevant differences. No native component is marked implemented or parity-reviewed.

## Next gate

Complete [native verification](issues/01-native-verification.md), then [independent visual references](issues/02-visual-reference.md). Keep the version at 0.1.0. Promote individual component statuses only when their evidence exists.
