# **rsync command** capable of using checksums stored in **trusted** extended attributes

This **rsync** command is the standard command but it includes specific code that enables the use of the
**-c** option (to use checksums instead of size and modification time) to decide whether files should be
transferred or not. However checksums are not computed if they have already been stored in **trusted
extended attributes** (trusted xattr).

When files are transferred, checksums (**MD5**, **SHA256** and **SHA512**) are computed on the fly.

Only **root** is allowed to use trusted extended attributes.

When used, MD5, SHA256 and SHA512 checksums are always computed simultaneously and stored in **trusted xattr**.

If needed, checksums may be computed beforehand with the **checksums** command, so that the execution of the
rsync command with the **-c** option will be much faster when there is nothing to do.


``` bash
Use "rsync --daemon --help" to see the daemon-mode command-line options.
Please see the rsync(1) and rsyncd.conf(5) manpages for full documentation.
See https://rsync.samba.org/ for updates, bug reports, and answers

This version includes the management of checksums in trusted extended attributes (for root).
[Sep 27 2026 17:54:22]

```

