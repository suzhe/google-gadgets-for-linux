**Q:** What is Google Gadgets for Linux?

**A:** It's a platform for running desktop gadgets under Linux. Please read QuickStart for a detailed introduction.


**Q:** What is Google's involvement with this project?

**A:** This is an official Google project, and is fully sponsored by the company.


**Q:** How to build and install Gadgets for Linux on Ubuntu 8.04?

**A:** For information on building on a particular distribution, please refer to the HowToBuild wiki page, as weel as the platform-specific instructions page on our user discussion list:

http://groups.google.com/group/google-gadgets-for-linux-user/web/building-instructions-addendum

Users are encouraged to share information on building Gadgets for Linux on that page.


**Q:** Can you provide a binary release of Gadgets for Linux?

**A:** You can refer to BinaryPackages wiki page.


**Q:** How to setup proxy for Google Gadgets for Linux?

**A:** Currently there is no GUI for proxy setup. But you can setup proxy by setting following environment variables before launching GGL:
```
http_proxy=http://yourproxy.com:8080
https_proxy=http://yourproxy.com:8080
```
or
```
all_proxy=http://yourproxy.com:8080
```
Replace yourproxy.com with your proxy address and 8080 with your proxy port.

You can also put the user name and password into the proxy URL like the following if the proxy server requires authentication:
```
all_proxy=http://username:password@yourproxy.com:8080
```

Many Linux distributions have GUI for changing system wide proxy settings. You can also use them. For example, on Ubuntu, you can find it in System->Preferences menu, named "Network Proxy".