# Contribution Guide {#contribution_guide}

## How to Contribute a Feature

1. Create an issue in this repository with a short summary of the feature and any specs you want to write. Summaries and specs are a great way to make sure everyone is on the same page before writing code. No one likes writing a feature only to have to rewrite or scrap it later due to a misunderstanding. Specs also make writing documentation like user guides easier.
2. [Fork this repository](https://help.github.com/en/articles/fork-a-repo) to create your own personal copy of the project. Working in your own forked repository guarantees no one else will mess with the project in unexpected ways during your development (and you won't mess with anyone else either!).
3. If you're going to develop multiple features in your fork, you should [create a branch](https://help.github.com/en/articles/creating-and-deleting-branches-within-your-repository) for each feature. Keeping a single feature per-branch leads to cleaner and easier to understand pull requests.
4. This is the fun part, write your feature!
    1. Please comment your code. This makes it easier for everyone to understand.
    2. [Doxygen comments](http://www.doxygen.nl/manual/docblocks.html) will be especially appreciated. You can [set up VSCode](https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen) to automatically add empty Doxygen templates to functions
    3. You should [run clang-format](https://clang.llvm.org/docs/ClangFormat.html) with [this project's .clang-format file](https://github.com/AEFeinstein/Super-2024-Swadge-FW/blob/main/.clang-format) to beautify the code. Everyone loves pretty code. There's a [VSCode Extension](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) for this too.
    4. The code should compile without any warnings.
    5. Please write small, useful messages in each commit.
5. Test your feature. Try everything, mash buttons, whatever. Get creative. Users certainly will. Write your test plan and steps in the ticket you opened.
6. Once your feature is written and tested, [create a pull request](https://help.github.com/en/articles/creating-a-pull-request) to merge the feature back to the master project. Please reference the ticket from step 1 in the pull request.
7. The new code will be reviewed and either merge it or have changes requested. The better the spec and conversation in step 1, the better the chances it gets merged quickly.

## How to Report a Bug

Create an issue in this project with the following information:

* What the expected behavior is.
* What the actual behavior is. Pictures or video could be useful here.
* If repeatable, steps to reproduce the buggy behavior.
* If not repeatable, what you were doing when the bug occurred.
* Serial logs, if you were logging.