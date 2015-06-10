In order to keep our code review safe,
you may use [GnuPG2](http://www.gnupg.org)
to sign your code review request.

# Installation #

For Debian and Ubuntu,
use this command to install GnuPG2:

```
sudo apt-get install gpg2
```

For other distributions,
please follow the instructions from
[GnuPG's homepage](http://www.gnupg.org)
to install GnuPG.

# Generating a Key #

Type `gpg2 --gen-key` under your command prompt,
and follow its instruction to create a key.
We recommend using 2048-bit RSA key to sign your code review.

Then you can export your key into ASCII format by this command:

```
gpg2 --export -a your_key_id
```

Then please post your key to our mailing list.
After examining the ownership of the key,
we'll add the key to the [members' public keys](GPGPublicKeys.md) page.

# Note #

Every leading minus sign in gpg-signed mail will be escaped with another
minus and a space.
So please run

```
sed 's/^- //'
```

to the received patch before applying it.
Otherwise, gpg will report that the patch is in wrong format.