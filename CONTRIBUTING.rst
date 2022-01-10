Contribution Guidelines
#######################

As an open-source project, we welcome and encourage the community to submit
patches directly to the project.  In our collaborative open source environment,
standards and methods for submitting changes help reduce the chaos that can result
from an active development community.

This document explains how to participate in project conversations, log bugs
and enhancement requests, and submit patches to the project so your patch will
be accepted quickly in the codebase.


Licensing
*********

Licensing is very important to open source projects. It helps ensure the
software continues to be available under the terms that the author desired.

.. _Apache 2.0 license:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/LICENSE

.. _GitHub repo: https://github.com/zephyrproject-rtos/zephyr

Zephyr uses the `Apache 2.0 license`_ (as found in the LICENSE file in the
project's `GitHub repo`_) to strike a balance between open contribution and
allowing you to use the software however you would like to. There are some
imported or reused components of the Zephyr project that use other licensing,
as described in `Zephyr Licensing`_.

.. _Zephyr Licensing:
   https://www.zephyrproject.org/doc/LICENSING.html

The license tells you what rights you have as a developer, provided by the
copyright holder. It is important that the contributor fully understands the
licensing rights and agrees to them. Sometimes the copyright holder isn't the
contributor, such as when the contributor is doing work on behalf of a
company.

.. _DCO:

Developer Certification of Origin (DCO)
***************************************

To make a good faith effort to ensure licensing criteria are met, the Zephyr
project requires the Developer Certificate of Origin (DCO) process to be
followed.

The DCO is an attestation attached to every contribution made by every
developer. In the commit message of the contribution, (described more fully
later in this document), the developer simply adds a ``Signed-off-by``
statement and thereby agrees to the DCO.

When a developer submits a patch, it is a commitment that the contributor has
the right to submit the patch per the license.  The DCO agreement is shown
below and at http://developercertificate.org/.


.. code-block:: none

    Developer's Certificate of Origin 1.1

    By making a contribution to this project, I certify that:

    (a) The contribution was created in whole or in part by me and I
        have the right to submit it under the open source license
        indicated in the file; or

    (b) The contribution is based upon previous work that, to the
        best of my knowledge, is covered under an appropriate open
        source license and I have the right under that license to
        submit that work with modifications, whether created in whole
        or in part by me, under the same open source license (unless
        I am permitted to submit under a different license), as
        Indicated in the file; or

    (c) The contribution was provided directly to me by some other
        person who certified (a), (b) or (c) and I have not modified
        it.

    (d) I understand and agree that this project and the contribution
        are public and that a record of the contribution (including
        all personal information I submit with it, including my
        sign-off) is maintained indefinitely and may be redistributed
        consistent with this project or the open source license(s)
        involved.

DCO Sign-Off Methods
====================

The DCO requires a sign-off message in the following format appear on each
commit in the pull request::

   Signed-off-by: Zephyrus Zephyr <zephyrus@zephyrproject.org>

The DCO text can either be manually added to your commit body, or you can add
either ``-s`` or ``--signoff`` to your usual Git commit commands. If you forget
to add the sign-off you can also amend a previous commit with the sign-off by
running ``git commit --amend -s``. If you've pushed your changes to GitHub
already you'll need to force push your branch after this with ``git push -f``.


Prerequisites
*************

.. _Zephyr Project website: https://zephyrproject.org

As a contributor, you'll want to be familiar with the Zephyr project, how to
configure, install, and use it as explained in the `Zephyr Project website`_
and how to set up your development environment as introduced in the Zephyr
`Getting Started Guide`_.

.. _Getting Started Guide:
   https://www.zephyrproject.org/doc/getting_started/getting_started.html

The examples below use a Linux host environment for Zephyr development.
You should be familiar with common developer tools such as Git and Make, and
platforms such as GitHub.

If you haven't already done so, you'll need to create a (free) GitHub account
on http://github.com and have Git tools available on your development system.

Repository layout
*****************

To clone the main Zephyr Project repository use::

    $ git clone https://github.com/zephyrproject-rtos/zephyr

The Zephyr project directory structure is described in `Source Tree Structure`_
documentation. In addition to the Zephyr kernel itself, you'll also find the
sources for technical documentation, sample code, supported board
configurations, and a collection of subsystem tests.  All of these are
available for developers to contribute to and enhance.

.. _Source Tree Structure:
   https://www.zephyrproject.org/doc/kernel/overview/source_tree.html

Pull Requests and Issues
************************

.. _Zephyr Project Issues: https://jira.zephyrproject.org

.. _open pull requests: https://github.com/zephyrproject-rtos/zephyr/pulls

.. _Zephyr-devel mailing list:
   https://lists.zephyrproject.org/mailman/listinfo/zephyr-devel

Before starting on a patch, first check in our Jira `Zephyr Project Issues`_
system to see what's been reported on the issue you'd like to address.  Have a
conversation on the `Zephyr-devel mailing list`_ (or the #zephyrproject IRC
channel on freenode.net) to see what others think of your issue (and proposed
solution).  You may find others that have encountered the issue you're
finding, or that have similar ideas for changes or additions.  Send a message
to the `Zephyr-devel mailing list`_ to introduce and discuss your idea with
the development community.

It's always a good practice to search for existing or related issues before
submitting your own. When you submit an issue (bug or feature request), the
triage team will review and comment on the submission, typically within a few
business days.

You can find all `open pull requests`_ on GitHub and open `Zephyr Project
Issues`_ in Jira.

Development Tools and Git Setup
*******************************

Signed-off-by
=============

The name in the commit message ``Signed-off-by:`` line and your email must
match the change authorship information. Make sure your *.git/config* is set
up correctly::

   $ git config --global user.name "David Developer"
   $ git config --global user.email "david.developer@company.com"

gitlint
=========

When you submit a pull request to the project, a series of checks are
performed to verify your commit messages meet the requirements. The same step
done during the CI process can be performed locally using the the *gitlint*
command.

Install gitlint and run it locally in your tree and branch where your patches
have been committed::

     $ sudo pip3 install gitlint
     $ gitlint

Note, gitlint only checks HEAD (the most recent commit), so you should run it
after each commit, or use the ``--commits`` option to specify a commit range
covering all the development patches to be submitted.

sanitycheck
===========

To verify that your changes did not break any tests or samples, please run the
``sanitycheck`` script locally before submitting your pull request to GitHub. To
run the same tests the CI system runs, follow these steps from within your
local Zephyr source working directory::

    $ source zephyr-env.sh
    $ make host-tools
    $ export PREBUILT_HOST_TOOLS=${ZEPHYR_BASE}/bin
    $ export USE_CCACHE=1
    $ ./scripts/sanitycheck

The above will execute the basic sanitycheck script, which will run various
kernel tests using the QEMU emulator.  It will also do some build tests on
various samples with advanced features that can't run in QEMU.

We highly recommend you run these tests locally to avoid any CI failures.
Using CCACHE and pre-built host tools is optional, however it speeds up the
execution time considerably.


Coding Style
************

Use these coding guidelines to ensure that your development complies with the
project's style and naming conventions.

.. _Linux kernel coding style:
   https://kernel.org/doc/html/latest/process/coding-style.html


In general, follow the `Linux kernel coding style`_, with the
following exceptions:

* Add braces to every ``if`` and ``else`` body, even for single-line code
  blocks. Use the ``--ignore BRACES`` flag to make *checkpatch* stop
  complaining.
* Use spaces instead of tabs to align comments after declarations, as needed.
* Use C89-style single line comments, ``/*  */``. The C99-style single line
  comment, ``//``, is not allowed.
* Use ``/**  */`` for doxygen comments that need to appear in the documentation.

The Linux kernel GPL-licensed tool ``checkpatch`` is used to check coding
style conformity. Checkpatch is available in the scripts directory. To invoke
it when committing code, edit your *.git/hooks/pre-commit* file to contain:

.. code-block:: bash

    #!/bin/sh
    set -e exec
    exec git diff --cached | ${ZEPHYR_BASE}/scripts/checkpatch.pl - || true

Contribution Workflow
*********************

One general practice we encourage, is to make small,
controlled changes. This practice simplifies review, makes merging and
rebasing easier, and keeps the change history clear and clean.

When contributing to the Zephyr Project, it is also important you provide as much
information as you can about your change, update appropriate documentation,
and test your changes thoroughly before submitting.

The general GitHub workflow used by Zephyr developers uses a combination of
command line Git commands and browser interaction with GitHub.  As it is with
Git, there are multiple ways of getting a task done.  We'll describe a typical
workflow here:

.. _Create a Fork of Zephyr:
   https://github.com/zephyrproject-rtos/zephyr#fork-destination-box

#. `Create a Fork of Zephyr`_
   to your personal account on GitHub.  (Click on the fork button in the top
   right corner of the Zephyr project repo page in GitHub.)

#. On your development computer, clone the fork you just made::

     $ git clone https://github.com/<your github id>/zephyr

   This would be a good time to let Git know about the upstream repo too::

     $ git remote add upstream https://github.com/zephyrproject-rtos/zephyr.git

   and verify the remote repos::

     $ git remote -v

#. Create a topic branch (off of master) for your work (if you're addressing
   Jira issue, we suggest including the Jira issue number in the branch name)::

     $ git checkout master
     $ git checkout -b fix_comment_typo

   Some Zephyr subsystems do development work on a separate branch from
   master so you may need to indicate this in your checkout::

     $ git checkout -b fix_out_of_date_patch origin/net

#. Make changes, test locally, change, test, test again, ...  (Check out the
   prior chapter on `sanitycheck`_ as well).

#. When things look good, start the pull request process by adding your changed
   files::

     $ git add [file(s) that changed, add -p if you want to be more specific]

   You can see files that are not yet staged using::

     $ git status

#. Verify changes to be committed look as you expected::

     $ git diff --cached

#. Commit your changes to your local repo::

     $ git commit -s

   The ``-s`` option automatically adds your ``Signed-off-by:`` to your commit
   message.  Your commit will be rejected without this line that indicates your
   agreement with the `DCO`_.  See the `Commit Guidelines`_ section for
   specific guidelines for writing your commit messages.

#. Push your topic branch with your changes to your fork in your personal
   GitHub account::

     $ git push origin fix_comment_typo

#. In your web browser, go to your forked repo and click on the
   ``Compare & pull request`` button for the branch you just worked on and
   you want to open a pull request with.

#. Review the pull request changes, and verify that you are opening a
   pull request for the appropriate branch. The title and message from your
   commit message should appear as well.

#. If you're working on a subsystem branch that's not ``master``,
   you may need to change the intended branch for the pull request
   here, for example, by changing the base branch from ``master`` to ``net``.

#. GitHub will assign one or more suggested reviewers (based on the
   CODEOWNERS file in the repo). If you are a project member, you can
   select additional reviewers now too.

#. Click on the submit button and your pull request is sent and awaits
   review.  Email will be sent as review comments are made, or you can check
   on your pull request at https://github.com/zephyrproject-rtos/zephyr/pulls.

#. While you're waiting for your pull request to be accepted and merged, you
   can create another branch to work on another issue. (Be sure to make your
   new branch off of master and not the previous branch.)::

     $ git checkout master
     $ git checkout -b fix_another_issue

   and use the same process described above to work on this new topic branch.

#. If reviewers do request changes to your patch, you can interactively rebase
   commit(s) to fix review issues.  In your development repo::

     $ git fetch --all
     $ git rebase --ignore-whitespace upstream/master

   The ``--ignore-whitespace`` option stops ``git apply`` (called by rebase)
   from changing any whitespace. Continuing::

     $ git rebase -i <offending-commit-id>^

   In the interactive rebase editor, replace ``pick`` with ``edit`` to select
   a specific commit (if there's more than one in your pull request), or
   remove the line to delete a commit entirely.  Then edit files to fix the
   issues in the review.

   As before, inspect and test your changes. When ready, continue the
   patch submission::

     $ git add [file(s)]
     $ git rebase --continue

   Update commit comment if needed, and continue::

     $ git push --force origin fix_comment_typo

   By force pushing your update, your original pull request will be updated
   with your changes so you won't need to resubmit the pull request.


Commit Guidelines
*****************

Changes are submitted as Git commits. Each commit message must contain:

* A short and descriptive subject line that is less than 72 characters,
  followed by a blank line. The subject line must include a prefix that
  identifies the subsystem being changed, followed by a colon, and a short
  title, for example:  ``doc: update wiki references to new site``.
  (If you're updating an existing file, you can use
  ``git log <filename>`` to see what developers used as the prefix for
  previous patches of this file.)

* A change description with your logic or reasoning for the changes, followed
  by a blank line.

* A Signed-off-by line, ``Signed-off-by: <name> <email>`` typically added
  automatically by using ``git commit -s``

* If the change address a Jira issue, include a line of the form::

      Jira: ZEP-xxx


All changes and topics sent to GitHub must be well-formed, as described above.

Commit Message Body
===================

When editing the commit message, please briefly explain what your change
does and why it's needed. A change summary of ``"Fixes stuff"`` will be rejected. An
empty change summary is only acceptable for trivial changes fully described by
the commit title (e.g., ``doc: fix misspellings in CONTRIBUTING doc``)

The description body of the commit message must include:

* **what** the change does,
* **why** you chose that approach,
* **what** assumptions were made, and
* **how** you know it works -- for example, which tests you ran.

For examples of accepted commit messages, you can refer to the Zephyr GitHub
`changelog <https://github.com/zephyrproject-rtos/zephyr/commits/master>`__.

Other Commit Expectations
=========================

* Commits must build cleanly when applied on top of each other, thus avoiding
  breaking bisectability.

* Commits must pass the *scripts/checkpatch.pl* requirements.

* Each commit must address a single identifiable issue and must be
  logically self-contained. Unrelated changes should be submitted as
  separate commits.

* You may submit pull request RFCs (requests for comments) to send work
  proposals, progress snapshots of your work, or to get early feedback on
  features or changes that will affect multiple areas in the code base.

Identifying Contribution Origin
===============================

When adding a new file to the tree, it is important to detail the source of
origin on the file, provide attributions, and detail the intended usage. In
cases where the file is an original to Zephyr, the commit message should
include the following ("Original" is the assumption if no Origin tag is
present)::

    Origin: Original

In cases where the file is imported from an external project, the commit
message shall contain details regarding the original project, the location of
the project, the SHA-id of the origin commit for the file, the intended
purpose, and if the file will be maintained by the Zephyr project,
(whether or not the Zephyr project will contain a localized branch or if
it is a downstream copy).

For example, a copy of a locally maintained import::

    Origin: Contiki OS
    License: BSD 3-Clause
    URL: http://www.contiki-os.org/
    commit: 853207acfdc6549b10eb3e44504b1a75ae1ad63a
    Purpose: Introduction of networking stack.
    Maintained-by: Zephyr

For example, a copy of an externally maintained import::

    Origin: Tiny Crypt
    License: BSD 3-Clause
    URL: https://github.com/01org/tinycrypt
    commit: 08ded7f21529c39e5133688ffb93a9d0c94e5c6e
    Purpose: Introduction of TinyCrypt
    Maintained-by: External
