# How to become a contributor and submit patches #

## Contributor License Agreements ##

We love to accept your code patches. However, before we can take them, we have to jump through a couple of legal hurdles.

Please fill out either the individual or corporate Contributor License Agreement:

  * If you are an individual writing original source code and you're sure you own the intellectual property, then you'll need to sign an [Individual CLA](http://code.google.com/legal/individual-cla-v1.0.html).
  * If you work for a company that wants to allow you to contribute your work to Google Gadgets for Linux, then you'll need to sign a [Corporate CLA](http://code.google.com/legal/corporate-cla-v1.0.html).

Follow either of the two links above to access the appropriate CLA and instructions on how to sign and return it. Once we receive it, we'll add you to the official list of contributors and will be able to accept your patches.

## Submitting Patches ##

It's very important that you sign the CLA mentioned above. Once you've done that, follow the steps below to submit patches:

  1. Join the [discussion group for developers](http://groups.google.com/group/google-gadgets-for-linux-dev). We can only add you after you've signed the CLA.
  1. Decide what code you want to submit. A submission should be a set of changes that addresses one issue in the issue tracker. Please don't mix more than one logical change per submission as it makes the history hard to follow. If you want to make a change that doesn't have a corresponding issue in the issue tracker, please create an issue first.
  1. Also, coordinate with team members that are listed on the issue in question. This ensures that work isn't being duplicated. Communicating your plan early also generally leads to better patches.
  1. Ensure that your code adheres to the source code style.
  1. Ensure that there are unit tests for your code.
  1. Attach the code to the corresponding issue you're trying to fix. For brand new files, attach the whole file. For patches to existing files, attach a Subversion diff (that is, `svn diff`).