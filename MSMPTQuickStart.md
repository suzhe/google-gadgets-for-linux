# Installation #

For Debian and Ubuntu,
use this command to install MSMTP:

```
sudo apt-get install msmtp
```

For other distributions,
please follow the instructions from
[MSMTP's homepage](http://msmtp.sourceforge.net)
to install MSMTP.

# Configuration #

MSMTP can read every parameter it needed from the command line.
But saving some of the parameters in the configuration file for MSMTP
can save you a lot of keystrokes.
The configuration file is `~/.msmtprc` and must not be readable to any
other users in your workstation.
Otherwise, MSMTP will refuse to run.
So you must set the permission of this file to 0600:

```
touch ~/.msmtprc
chmod 0600 ~/.msmtprc
```

Here is a recommended sample configuration of `MSMTP`:

```
# Use an external SMTP server with insecure authentication.
# (manually choose an insecure authentication method.)
# Note that the password contains blanks.

defaults

######################################################################
# A sample configuration using Gmail
######################################################################

# account name is "gmail".
# You can select this account by using "-a gmail" in your command line.
account gmail
host smtp.gmail.com
tls on
tls_certcheck off
port 587
auth login
from somebody@gmail.com
user somebody
password somesecret

######################################################################
# A sample configuration using other normal ESMTP account
######################################################################

# account name is "someplace".
# You can select this account by using "-a someplace" in your command line.
account someplace
host smtp.someplace.com
from someone@someplace.com
auth login
user someone
password somesecret

# If you don't use any "-a" parameter in your command line,
# the default account "someplace" will be used.
account default: someplace
```

# Test #

Now you can do a simple test to ensure your configuration is correct.
Copy and paste these lines to your command line prompt,
modifying the "-a" parameter to your desired account and the E-mail
address to your own address:

```
cat <<EOF | msmtp -a gmail someone@gmail.com
Subject: test

This is a test!
EOF
```

If you're sending from a Gmail account, take a look at the "Sent Mail" box on the sending account - Gmail will add the message to that box, but not deliver the
mail to your inbox if the sender and recipient are identical.
The message should also appear in your receiving mailbox.
If the mail can be found in the expected mailboxes, then everything is successfully configured.