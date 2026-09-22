# Security

The library is experimental. No production-support or security-audit claim applies to 0.1.0.

For an ordinary crash, open a minimal reproducer after the repository is published. For a suspected exploitable memory-safety issue, use GitHub's private vulnerability reporting when the maintainer enables it. Do not put secrets or a working exploit against another application in a public issue.

Widget calls belong on the GUI thread. A returned widget reference is borrowed. Do not retain it past its parent's lifetime or combine Qt parent ownership with an owning smart pointer. Use QPointer for a delayed observer and a receiver context for signal callbacks.

Sanitizers and static analysis can find defects; they do not prove memory safety. The Qt-free tests do not exercise Qt, display drivers, font engines or platform accessibility implementations.
