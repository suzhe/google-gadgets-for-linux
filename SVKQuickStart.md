Every member here is highly recommended to use
[SVK](http://svk.bestpractical.com/)
to participate in the development of this project.
This document is for the developers who are already familiar with
[Subversion](http://subversion.tigris.org/),
but have never used [SVK](http://svk.bestpractical.com/).
If you are not familiar with
[Subversion](http://subversion.tigris.org/) either,
please consider reading the
[Subversion Book](http://svnbook.red-bean.com/) first.

# Installation #

For Debian and Ubuntu,
use this command to install SVK:

```
sudo apt-get install svk
```

For other distributions,
please follow the instructions from
[SVK's homepage](http://svk.bestpractical.com/)
to install SVK.

# Setting Up #

In order to use SVK to participate in this project,
follow these steps to initialize your working
environment.

## Initializing ##

Type this command in your virtual terminal (command line prompt):

```
svk depotmap
```

SVK will launch a text editor containing something like this, where
`yourname` is your username:

```
---
"": /home/yourname/.svk/local

==edit the above depot map===
```

Add this line after the second line:

```
ggl: /home/yourname/.svk/google-gadgets-for-linux
```

Then save and exit the text editor.
Now you have successfully created a local SVK repository.
We'll mirror the remote SVN repository into your local SVK
repository in the next section.

## Mirroring ##

First, tell SVK the location of the remote SVN repository:

```
svk mirror /ggl/mirror https://google-gadgets-for-linux.googlecode.com/svn/
```

Then synchronize the local mirror with the remote repository:

```
svk sync /ggl/mirror
```

Now you can leave your computer and have a cup of coffee.
After a not-so-short wait,
you'll have the entire SVN repository
(including all change history)
mirrored in your local SVK repository.

## Local Copy ##

Although `/ggl/mirror` is created on your computer,
every operation under this directory is automatically synchronized with the
remote SVN repository.
Therefore it's preferable to
create another local branch in which to work.
Branching operation in SVK is a simple copy operation, the same as in SVN:

```
svk cp /ggl/mirror /ggl/local
```

Like SVN,
SVK will then launch a text editor
asking for the committing log.
Just type something reasonable and then save and exit the text editor.

## Checking Out ##

Now you can check out code from your local working branch into
your working directory.
For this, follow a similar procedure as that of SVN:

```
mkdir ggl
svk co /ggl/local/trunk ggl/trunk
svk co /ggl/mirror/wiki ggl/wiki
```

Note: as we are not going to do the code review process on the wiki pages, we simply check out the wiki pages from the `mirror`, not the `local`.

## Utility Scripts ##

This project provides two simple utilities.
`svkmail` is used for sending out code review requests.
`svkcommit` is used for checking
files that you forgot to import into the version control system before
committing your change list.
We highly recommend you using these tools during your development.

For your convenience,
please link these scripts to one of your `$PATH` searching directories -
for example,
`~/bin`.
Then you can run these command more easily:

```
mkdir -p ~/bin
ln -s ggl/trunk/utils/svkmail ~/bin/svkmail
ln -s ggl/trunk/utils/svkcommit ~/bin/svkci
```

`svkmail` can read every parameter it needed from the command line.
But saving some of the parameters in the configuration file for
`svkmail` can save you a lot of keystrokes.
The configuration file is named `.svkmail` and must be saved at the top
of your checking out directory.
`svkmail` searches for the configuration file in the current working
directory and each of the current working directory's successively
higher parent directories until the file is located.
Therefore you don't need to
go to the top level of your checkout directory before running
this script.
You should put your `.svkmail` in both `ggl/trunk/` and `ggl/wiki/`.

Here is a recommended sample configuration of `svkmail`:

```
# Your E-mail address. Used for sending out code review requests.
from=change@this

# Send review request to this address.
# Note: different modules need to be reviewed by different people.
#       So we recommended that don't uncomment this line, just using
#       -m parameter in the command line every time.
#m=change@this

# Cc a copy of the review request to the mailing list.
cc=google-gadgets-for-linux-dev@googlegroups.com

# Command used for sending your mails.
# By default, svkmail uses "sendmail".
# If you want to use other command,
# the command must be sendmail-compatible.
# A recommended choice is msmtp.
sendmail="msmtp -a gmail -t"

# Command used for signing your mails.
# A recommended choice is GnuPG2
sign="gpg2 --clearsign -u your_key_id"
```

For more information about `msmtp` and `gpg2`,
see [MSMTP Quick Start](msmtp_quick_start.md) and
[GnuPG2 Quick Start](gpg_quick_start.md).

OK! Everything is ready now.
Just say "hooray" and then you could start working.

# Working On Your Linux Box #

During your tenure as a developer on this project,
a typical routine is "syncing", "merging", "updating",
"modifying", "committing", "code reviewing" and "submitting".

## Syncing ##

Each time before you start work,
you should synchronize the local repository with the latest status of the
SVN repository. Simply type:

```
svk sync /ggl/mirror/
```

This command will download the latest changes made by other
developers.
If nothing is new since the last sync,
you can skip the "merging" step.
Otherwise you will need to merge the changes from your local mirror to your
local branch.

## Merging ##

This command will merge the changes from your local mirror to your local
branch:

```
svk smerge /ggl/mirror/ /ggl/local/
```

But in most cases,
we don't want the synchronize the entire `local` repository with the
whole `mirror` repository which including all tags and branches,
so maybe you only want to synchronize a subset of the branches:

```
svk smerge /ggl/mirror/trunk/ /ggl/local/trunk/
svk smerge /ggl/mirror/wiki/ /ggl/local/wiki/
```

## Updating ##

Every changes is already on your local branch.
Now just updating the latest changes to your working directory:

```
svk up
```

## Modifying ##

Now feel free to modify your sources.
These commands may be useful during your creative work:

```
svk add filename
svk rm filename
svk revert filename
svk status
svk diff
```

## Committing ##

When you finish coding,
you can commit your code into your local working branch
using this command in preparation for a code review:

```
svk commit
```

A text editor will be launched to ask you for the commit log.
Be sure to note the names of the code reviewers on the commit log,
using this format:

```
Reviewer: reviewer@somehost
```

Or

```
Reviewers: reviewer1@somehost1, reviewer2@somehost2
```

Committing to the local working branch will not affect the remote SVN
repository at all,
so feel free to commit any changes to your local working branch.
No one else will be disturbed by you.

## Code Reviewing ##

After committing to your local working branch,
you could now send out a code-review request to the other developers.
Although the SVN server allows directly
committing your changes,
committing to the SVN server without a code-review request is strictly
prohibited in our group.
Any member doing so may be kicked out from the committer list.
Use this command to generate a code-review request:

```
svkmail -m reviewer@somehost
```

If you've committed many changes to your local working branch,
you can send out a single code-review request containing all of those
changes. For example, this command sends out a review request from local
[revision 105](https://code.google.com/p/google-gadgets-for-linux/source/detail?r=105) to 108:

```
svkmail -m reviewer@somehost -r 105:108
```

## Submitting ##

If other developers replied to your request and gave positive comments
to your changes,
you can then submit the changes to the SVN repository.
There are three ways to submit changes.

### Automatically ###

Type this command:

```
svk smerge /ggl/local/trunk/ /ggl/mirror/trunk/
```

This command merges all changes in local branch to the local mirror.
And every change to the local mirror will be applied to the SVN
repository automatically.
A text editor will be launched asking you for the commit log.
Be sure to note the names of the code reviewers on the commit log.

### Incrementally ###

If you have committed multiple changes to your local branch and want
these changes be committed to the SVN repository separately,
instead of merging all the changes into one change list,
you can use this command:

```
svk smerge -I /ggl/local/trunk/ /ggl/mirror/trunk/
```

This command will merge your changes incrementally,
using your previous commit logs and will not prompt for any additional logs.

### Manually ###

The best way to commit code to SVN is to manually commit changes.
This can be done in 5 steps:

First,
switch to the local mirror:

```
svk switch /ggl/mirror/trunk/
```

Then merge the changes from the local branch:

```
svk merge /ggl/local/trunk/ -r M:N .
```

Now you can try compiling the code and
running the unit test
to ensure that the code you committed is OK.
Then submit your changes:

```
svk ci
```

As we are already working on the local mirror,
every change to it will be applied to the SVN server,
too.
So we've already updated the SVN repository on server side.

Then we should merge the changes from the mirror to the local branch:

```
svk smerge /ggl/mirror/trunk/ /ggl/local/trunk/
```

At last,
switch back to the local branch:

```
svk switch /ggl/local/trunk/
```

# FAQ #

**Q:**
I ran `svk ci` in preparation for a code review,
but another developer checked in his code.
Now in order to send out a new up-to-date review,
my change list will include the changes made by the other developer.
How do I send out a clean code review in this case?

**A:** It's possible to send out a code review request
without merging the remote changes in order to avoid this problem.
But the out-of-dated patch will certainly confuse your reviewers.
So we strongly suggest that you merge before you send out a code review request,
leaving the troubles to yourself only.
After merging the changes,
you must use this command to send out the code review request:

```
svkmail -m reviewer@host -r M:N -genpatch "svk diff /ggl/mirror/trunk/ /ggl/local/trunk/"
```

Note: You'll not be able to do incrementally committing in this situation.
Therefore try to avoid this problem altogether by making your changes small,
reducing the number of pending changes,
and submitting more often.

If you have any questions about SVK,
please contact us via the mailing list.