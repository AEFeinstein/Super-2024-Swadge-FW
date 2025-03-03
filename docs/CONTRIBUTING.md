# Contribution Guide {#contribution_guide}

## How To Create Github Issues

Work is tracked through [GitHub Issues here](https://github.com/AEFeinstein/Super-2024-Swadge-FW/issues). Issues are much less likely to get lost than Slack threads. Issues should be appropriately labeled and assigned to someone. Issues can be linked to Pull Requests and should be closed when the work is done, which may take multiple Pull Requests. We're currently not using any project management tracking more sophisticated than Issues, so please fill them out. There are Issue templates too, which make creation easier!

### Feature Requests

When you want a new feature added to the Swadge, use the Feature Request template. There are five sections in the template to consider, and you can add more if you want.

* Summary - What is the feature all about? Is it a game, a utility, an enhancement, or something else?
* Technical Spec - How will this be implemented? What peripherals will be used, and how?
* UI - What are the display screens & menus (if applicable)? What user inputs are there for each screen?
* Mockups - Paste any mockups of screens or anything else here.
* How to Test - How can we verify the feature is working?

Summaries and specifications are a great way to make sure everyone is on the same page before writing code. No one likes writing a feature only to have to rewrite or scrap it later due to a misunderstanding. Specifications also make writing documentation like user guides easier.

### Bug Reports

When you find a bug that needs to be fixed, use the Bug Report template. There are seven sections in the template to consider.

* Description - A clear and concise description of what the bug is.
* Expected behavior - A clear and concise description of what you expected to happen.
* Reproduction Steps - Steps to reproduce the behavior and how often it occurs.
* Screenshots - If applicable, add screenshots to help explain your problem.
* Logs - If applicable, add logs from when the problem occurred.
* Affected Version - The platform, git branch, and git hash the bug occurred on.
* Additional Notes - Add any other notes about the problem.

### Other Issues

If there's firmware work that's neither a feature nor a bug, you can start with a blank Issue. Please write a thorough description so the whole team can understand and help.

## How to Contribute a Feature

1. [Create a Feature Request Issue](https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/creating-an-issue) in this repository as noted above. This is a great place to discuss the feature and have any questions answered before writing any code.
2. If you aren't already a collaborator in this repository, ask on Slack to be added. We've found that it's easier to manage many branches in this single repository than across many forks and branches.
    * Alternatively, if you want to do development for yourself and not contribute it to official Swadge firmware, [fork this repository](https://help.github.com/en/articles/fork-a-repo) to create your own personal copy of the project.
3. [Create a branch](https://help.github.com/en/articles/creating-and-deleting-branches-within-your-repository) for the feature you want to develop. Keeping a single feature per-branch leads to cleaner and easier to understand pull requests. 
4. [Link the branch to the Github Issue](https://github.blog/changelog/2022-09-07-link-existing-branches-to-an-issue/) you made in step one.
5. This is the fun part, write your feature!
    1. Please comment your code. This makes it easier for everyone to understand.
    2. [Doxygen comments](http://www.doxygen.nl/manual/docblocks.html) will be especially appreciated. You can [set up VSCode](https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen) to automatically add empty Doxygen templates to functions.
    3. You should [run clang-format](https://clang.llvm.org/docs/ClangFormat.html) with [this project's .clang-format file](https://github.com/AEFeinstein/Super-2024-Swadge-FW/blob/main/.clang-format) to beautify the code. Everyone loves pretty code. You can run `make format` or there's a [VSCode Extension](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) for this too.
    4. The code should compile without any warnings. Be sure to compile both the emulator (`make all`) and firmware (`idf.py build`).
    5. Please write small, useful messages in each commit.
6. Test your feature. Try everything, mash buttons, whatever. Get creative. Users certainly will. Write your test plan and steps in the ticket you opened.
7. Once your feature is written and tested, [create a pull request](https://help.github.com/en/articles/creating-a-pull-request) to merge the feature back to the master project. Please reference the ticket from step 1 in the pull request.
8. The new code will be reviewed and either merge it or have changes requested. The better the spec and conversation in step 1, the better the chances it gets merged quickly.
