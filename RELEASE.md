# Release Process:
This document describes the general process that maintainers must follow when making a release of the `tabrmd`.

# Versioning Scheme
Our releases will follow the semantic versioning scheme.
You can find a thorough description of this scheme here: semantic versioning.
In short, this scheme has 3 parts to the version number: A.B.C

* A is the 'major' version, incremented when an API incompatible change is made
* B is the 'minor' version, incremented when an API compatible change is made
* C is the 'micro' version, incremented for bug fix releases
Please refer to the Semantic Versioning website for the authoritative description.

# Static Analysis
Before a release is made the `coverity_scan` branch must be updated to the point in git history where the release will be made from.
This branch must be pushed to github which will cause the travis-ci infrastructure to run an automated coverity scan.
The results of this scan must be dispositioned by the maintainers before the release is made.

# Git Tags
When a release is made a tag is created in the git repo identifying the release by version number e.g. A.B.C where A, B and C are integers representing the major, minor and micro components of the version number.
The tabrmd build system uses these tags to generate the version string used in the generation of the release tarball and so the tag must be created as the first step in the release process.
After the tag has been created the `distclean` make target should be executed, followed by the `bootstrap` script to regenerate the `configure` script.
The tag should be pushed to upstream git repo as the last step in the release process.
**NOTE** that tags of this form should be considered immutable:
If you are maintaining a build system / product that uses a specific version of this software you can rely on these tags to remain unchanged.

## Signed tags
Git supports GPG signed tags and for releases after the `1.1.0` release will have tags signed by a maintainer.
For details on how to sign and verify git tags see: https://git-scm.com/book/en/v2/Git-Tools-Signing-Your-Work.

## Release Candidates
In the run up to a release the maintainers may create tags to identify progress toward the release.
In these cases we will append a string to the release number to indicate progress using the abbreviation `rc` for 'release candidate'.
This string will take the form of '_rcX'.
We append an incremental digit 'X' in case more than one tag is necessary to communicate progress as development moves forward.
**NOTE** that tags of this form will be removed after a release with the corresponding version number has been made.

# Release tarballs
We use the git tag as a way to mark the point of the release in the projects history.
We do not however encourage users to build from git unless they intend to modify the source code and contribute to the project.
For the end user we provide release "tarballs" following the GNU conventions as closely as possible.

To make a release tarball use the `distcheck` make target.
This target incldues a number of sanity checks that are extremely helpful.
For more information on `automake` and release tarballs see: https://www.gnu.org/software/automake/manual/html_node/Dist.html#Dist

Release tarballs may be created for release candidates but this is not currently a requirement of this process.

## Hosting Releases on Github
Github automagically generates a page in their UI that maps git tags to 'releases' (even if the tag isn't for a release).
Additionally they support hosting release tarballs through this same interface.
The release tarball created in the previous step must be posted to github using the release interface.
Additionally this tarball must be accompanied by a detached GPG signature.
The Debian wiki has an excellent description of how to post a signed release to Github here: https://wiki.debian.org/Creating%20signed%20GitHub%20releases

# Signing Keys
The GPG keys used to sign a release tag and the associated tarball must be the same.
Additionally they must belong to a project maintainer and must be discoverable using a public GPG key server.

# Announcements
Release candidates and proper releases should be announced on the 01.org TPM2 mailing list: https://lists.01.org/mailman/listinfo/tpm2.
This announcement should be accompanied by a link to the release page on Github as well as a link to the CHANGELOG.md accompanying the release.
