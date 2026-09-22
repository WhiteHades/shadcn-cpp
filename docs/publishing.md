# Publish the repository

The authoring session had a read-only GitHub connector, so no remote repository, issue, release or Pages site was created. This source snapshot includes a guarded GitHub CLI script for publication from your machine.

Extract the source archive, enter the directory, sign in to GitHub CLI as WhiteHades and run:

```sh
gh auth login
bash tools/publish.sh --public
```

Use `--private` instead to start privately. The script checks the account and project records, refuses an existing repository or origin remote, creates the repository and pushes the current source. An extracted source archive needs your Git author name and email configured before its initial commit. A clone of the supplied Git bundle already contains the local commit history.

The script sets repository topics but does not create a release tag, force-push or enable Pages. Its network actions have not run in this environment. Inspect the first Actions runs after publication and fix failures before promoting any component status.

Run the documentation build after dependencies are available. Review and commit the generated npm lockfile. Pages deployment is a separate manual workflow, available once GitHub Pages is configured to use Actions. There is no currently published documentation URL.
