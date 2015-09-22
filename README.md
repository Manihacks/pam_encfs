# Pluggable Authentication Module to mount your encfs directories at login

# Description
This is a small pam module for old style login lovers
(those like me that logs through command line and start X with `startx`)
It actually allows you to mount an ENCFS directory at login
time. I know there was a previous pam_encfs module but I found it
quite old and did not fit to my needs so I decided to code my 
own. Since pam modules are quite difficult to debug, some bugs may
left. Please fill out bug report if you find something.

# How it works
1) pam_encfs tries to mount `/home/.$USER/` into `/home/$USER/` with the 
following encfs/fuse options `allow-root,nonempty` and will prompt
you for your password to decrypt it (does not use user's password
as it was in the old pam_encfs).
2) According to the checks performed before mounting attempt, if
`/home/.$USER/` is not a valid encfs target there is a fallback.
We parse the config file located at `/home/$USER/.pam_encfs` and
try to mount the directories specified in this file, with the same
options as above (i.e. `allow-root,nonempty`). Your password will
be prompted for every directory to mount.
3) When you log out, the module unmount everything that was mounted
with pam_encfs at login.

# Config file format
```
# This is a comment
# This tries to mount encfs target /home/dev/.crypted into /home/dev/decrypted
/home/dev/.crypted:/home/dev/decrypted
``` 

# Requirements
- encfs is installed
- user credentials are set properly (belongs to fuser group if needed)
- pam_encfs.so has been put at the right place (according to your distro)
and pam configured appropriately

# Known Issues
I had a lot of troubles in successfuly executing `startx` from my
decrypted directory. I had the following error *Error in locking authority 
file .Xauthority*, so I started to debug the thing. I pass you all the 
details on how I found out the solution because it might not interest you.
This was not easy to debug since this *locking* error comes because the function 
`XauLockAuth` fails to `link()` a file --> oO <-- and not because encfs does not support
locking as I have found in many places over the Internet. Anyway this was because
I activated "External IV chaining" when I configured my encfs directory. This
is also the default for *paranoid* mode when you set up your encfs directory.
So to prevent this to happen, just configure your encfs directory in *expert mode*
and answer "no" when it prompts you to enable "External IV chaining". 

NB: *External IV chaining* does not allow to create hard links (man encfs)

# Installation
1) Install *libpam-dev* (depends on your distro)
    - Debian: sudo apt-get install libpam-dev
    - Arch: 
2) Install *encfs*
3) Edit `/etc/fuse.conf` and uncomment the line with `user_allow_other`
4) Go in project root and do: `make build`
5) `sudo cp build/pam_encfs.so $PAM_DIR` with $PAM_DIR depending on your distro.
    - Debian: PAM_DIR=/lib/x86_64-linux-gnu/security/ or /lib/i386-linux-gnu/security/pam_unix.so
    - Arch: /lib/security
    - Others: find same dir as pam_unix.so ( `find / -name pam_unix.so` )
6) Edit `/etc/pam.d/$LOGIN` and append a new line with `session optional pam_encfs.so`
    - Debian: LOGIN_SESSION=/etc/pam.d/common-session
    - Arch: LOGIN_SESSION=/etc/pam.d/...
7) It should work


# Notes
- It seems not to work with graphical login (Gnome 3 tested)
- Not working with ssh-login, here is a quick workaround:
    ** login via ssh
    ** `su` to the user you want

# Successfully tested on
- Distributor ID: Debian Description: Debian GNU/Linux 7.8 (wheezy) Release: 7.8 Codename: wheezy
    ** Distributor ID: Debian Description: Debian GNU/Linux 7.8 (wheezy) Release: 7.8 Codename: wheezy
    Linux version 3.2.0-4-486 (debian-kernel@lists.debian.org) (gcc version 4.6.3 (Debian 4.6.3-14) ) #1 Debian 3.2.68-1+deb7u3
    ** Distributor ID: Debian Description: Debian GNU/Linux 7.8 (wheezy) Release: 7.8 Codename: wheezy
Linux version 3.2.0-4-amd64 (debian-kernel@lists.debian.org) (gcc version 4.6.3 (Debian 4.6.3-14) ) #1 SMP Debian 3.2.68-1+deb7u3
