# Decision 003: Docusaurus guides and Doxygen reference

Status: configured, build pending.

Docusaurus builds Markdown guides into a documentation site with navigation, code blocks and light/dark styling. Doxygen extracts the C++ declarations and comments. Doxygen writes the generated reference into the site's static API directory during the documentation build.

Docusaurus was selected as an established documentation-focused project. The GitHub pages checked during selection showed about 66,300 stars for Docusaurus and 27,500 for Material for MkDocs. That is a dated comparison of those options, not proof of a universal popularity ranking. The requirements are an open-source tool, readable output and a maintained build system, rather than stars alone.

Direct site dependencies are pinned. A resolved npm lockfile is still required. The authoring environment cannot reach the package registry, so no lockfile or successful site build has been fabricated. The first connected build generates a lockfile for review, after which CI uses npm ci.

[Docusaurus](https://docusaurus.io/docs) describes the guide-site tooling. [Doxygen](https://www.doxygen.nl/manual/starting.html) describes API generation. Their upstream repositories are [facebook/docusaurus](https://github.com/facebook/docusaurus) and [doxygen/doxygen](https://github.com/doxygen/doxygen).
