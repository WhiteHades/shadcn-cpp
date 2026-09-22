# Decision 002: pin one reference profile first

Status: accepted for the initial implementation.

The reference is shadcn-ui/ui commit `98a1fe67b439324ddc857f47fbdce056600a4329`, New York v4, neutral theme and a 16-pixel reference rem. The manifest records the exact source objects read for the first ten drafts.

Starting with a named profile makes screenshots and state comparisons meaningful. Alternate profiles and new upstream revisions require separate review. Common component names do not establish identical geometry or behaviour.

A source hash establishes which wrapper was inspected. It does not establish that an external behaviour package was ported or that a C++ rendering matches the browser. Those are separate evidence categories.
