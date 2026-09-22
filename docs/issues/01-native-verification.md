# Compile and verify the native increment

Status: planned.

## Dependencies

None.

## Scope

Build the existing ten widgets, gallery and tests before adding controls. Fix compiler errors, runtime failures and ownership defects through public-interface regression tests. Add an installed widgets consumer.

## Acceptance

GCC, MSVC and Apple Clang jobs build. QtTest passes. Linux sanitizer checks pass or record a reviewed platform-specific finding. The installed native consumer runs. Record exact Qt kits and results.

## Evidence

Native build logs, test output and the installed-consumer result. Do not promote visual or accessibility status from these checks.

The shared specification, source pin and ownership rules apply. This file is the local tracker record; no GitHub issue has been created.
