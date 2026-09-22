# Verification

Recorded on 22 September 2026. Version: 0.1.0.

This is the first local development snapshot, not a published release. The GitHub tools exposed in the authoring session provided reads but no repository creation or push operation. No remote repository, issue or Pages deployment was created.

## Checks that ran

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

## Checks that could not run

| Check | Status |
| --- | --- |
| Native Qt configuration and compilation | Blocked. The environment has no Qt 6 SDK. [CMake error](evidence/native-configure.txt). |
| Native unit tests and gallery rendering | Not run. They require the missing Qt SDK. |
| Native memory-safety checks | Not run. The configured Linux job still needs its first execution. |
| Browser-to-native visual and animation comparisons | Not run. No approved reference capture set exists yet. |
| Screen readers, input methods and desktop compositors | Not run. |
| Doxygen and Docusaurus build | Blocked. Doxygen and the site dependencies are unavailable. [Build prerequisite error](evidence/documentation.txt). |
| npm dependency resolution and lockfile | Not run. No fabricated lockfile is included. |
| Full checkout-based upstream audit | Not run. The exact source files and Git blob identifiers were read through GitHub; the audit tool ran against a temporary local fixture repository. |
| GitHub Actions and Pages | Configured only. No remote run or deployment occurred. |
| Public repository creation and push | Not performed. The publishing script requires an authenticated GitHub CLI with write permission on your machine. |

## Source review

The initial wrapper review covered Button, Card, Input, Badge, Label, Checkbox, Switch, Separator, Progress and Skeleton. The reference is shadcn-ui/ui commit `98a1fe67b439324ddc857f47fbdce056600a4329`, New York v4 and the neutral theme. The registry and licence were also read. The manifest records exact paths and hashes.

Manual review found missing Enter activation, incomplete custom-control hit areas and focus-frame issues during parenting. The source now includes corrections and native regression cases. Those corrections have not been runtime-verified. The focus-frame review also read Qt's [v6.8.3 implementation](https://github.com/qt/qtbase/blob/v6.8.3/src/widgets/widgets/qfocusframe.cpp).

Known gaps include shadows, some transitions and states, link-badge composition, vertical progress options, external behaviour dependencies, precise icon/font matching, colour gamut mapping and platform accessibility. The component records list the relevant differences. No native component is marked implemented or parity-reviewed.

## Next gate

Complete [native verification](issues/01-native-verification.md), then [independent visual references](issues/02-visual-reference.md). Keep the version at 0.1.0. Promote individual component statuses only when their evidence exists.
