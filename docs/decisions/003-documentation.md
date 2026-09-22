# Decision 003: Docusaurus with clang-doc

Status: configured.

Docusaurus builds the guides and C++ API reference into one site with shared navigation, code blocks and light/dark styling. clang-doc extracts declarations and comments from the two public headers. A small formatter adapts its Markdown for the site, fixes enum tables and constructor headings, and removes Qt's generated metadata members.

The API overview and ownership examples remain handwritten. Generated pages are rebuilt from the current headers. clang-doc's Markdown output omits some declaration details, including default arguments and qualifiers, so the reference links to the headers for exact declarations.

Site dependencies are locked in `website/package-lock.json`. Local and CI builds use `npm ci --ignore-scripts` followed by `npm run build`. Building the reference also requires Clang, clang-doc, CMake, Ninja and Qt. These tools are documentation build dependencies; applications only link the C++ library and Qt.

[Docusaurus](https://docusaurus.io/docs) documents the site tooling. [clang-doc](https://clang.llvm.org/extra/clang-doc.html) documents C++ extraction.
