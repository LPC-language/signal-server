Upgrading to a newer version of the software generally involves the following
steps:

  * update the signal-server and cloud-server repositories
  * login with your personal admin account and do the following:
    ```
    > compile /usr/MsgServer/inid.c
    $0 = </usr/MsgServer/initd>
    > code "/usr/MsgServer/initd"->upgrade()
    $1 = 1
    > upgrade -a /kernel/lib/auto.c /kernel/sys/driver.c
    ```
    If anything goes wrong during the last command, fix any errors and try
    again.

## 0.9 => 0.9.1

Before upgrading, add a line like
```
# define UPDATES_SERVER        "updates2.dworkin.nl"
```
to `src/config/services`.

After upgrading, update your clients with the new
`src/config/ZKGROUP_SERVER_PUBLIC_PARAMS`, which was regenerated to fix a bug.
