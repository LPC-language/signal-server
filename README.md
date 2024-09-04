# A simple, scalable reimplementation of Signal-Server v9.60

[Signal-Server](https://github.com/signalapp/Signal-Server) is famously
difficult to configure and install.  With dependencies on resources in Amazon
Web Services and Google Cloud Platform, it is also expensive to run.  This
makes it a poor choice for a private messenger server for e.g. a family or a
company.

Now there is a new option: `signal-server` rewritten from scratch in
[LPC](https://en.wikipedia.org/wiki/LPC).  The reimplementation runs on a
single host and stores all data in its internal database.  The LPC platform can
be either [DGD](https://github.com/dworkin/dgd) or
[Hydra](https://www.dworkin.nl/hydra); on the latter, it will scale with the
number of CPU cores on the host.

LPC is a simple language permitting rapid development.  Getting the
reimplementation of `signal-server` to the point where users can exchange
encrypted messages took about 8 months.  Advanced features of the underlying
platform, such as upgrading any LPC code without downtime, and even upgrading
the platform itself without downtime, are enabled for `signal-server`.

`signal-server` attempts to emulate the original Signal-Server v9.60.  It also
implements various dependencies of Signal-Server, including
[libsignal](https://github.com/signalapp/libsignal) v0.23.1, a REST framework,
and some components that were spun out of Signal-Server, such as the
registration service and CDSI.

`signal-server` has the following dependencies:

-   [DGD](https://github.com/dworkin/dgd) 1.7.5 or
    [Hydra](https://www.dworkin.nl/hydra) 1.3.37  
    The LPC compiler, runtime and database management system.
-   [cloud-server](https://github.com/dworkin/cloud-server) 1.1  
    The basic LPC framework providing HTTP and TLS support.
-   [LPC extension modules](https://github.com/dworkin/lpc-ext) 1.4.8  
    The `crypto` extension module is required for TLS.  Not required but highly
    recommended is the `jit` extension module for JIT compiler support.
-   [libsignal extension module](https://github.com/LPC-language/libsignal)  
    Cryptographic primitives for `signal-server`, roughly equivalent to
    [curve25519-dalek](https://github.com/signalapp/curve25519-dalek)
    3.0.0-lizard2.

The clients that match Signal-Server v9.60 are
[Signal-Android](https://github.com/signalapp/Signal-Android) v6.20.6 and
[Signal-iOS](https://github.com/signalapp/Signal-iOS) 6.23.1.0.  Only
[signal-android](https://github.com/LPC-language/signal-android) is supported
for now, with minimal changes made.

## Disclaimers

*This is alpha quality code*.  You can send simple encrypted messages to other
users, and that's it.  However, the foundations are there for more advanced
features, including support for Zero Knowledge Proofs.  A lot more could be
implemented in just a month of development time.

*I am not a cryptographer*.  For example, the cryptographic primitives are
not implemented as constant-time, even when the provided example code in
the Ristretto255 documentation is; rate-limiting is not implemented for any
REST endpoint; protocol errors are not carefully checked for.  All that
remains to be implemented, and then a real cryptographer should have a look.

## What works

A slightly modified Android client can register with the server, discover
other registered clients, and exchange end-to-end encrypted messages with
them.  Google FCM is used to notify clients of new messages.

## What doesn't work

Only that part of the REST API which the client uses to get to the
aforementioned point is implemented, with one exception: the
[storage service](https://github.com/signalapp/storage-service) API is not
implemented, and always returns HTTP status 401.  The API endpoint to obtain
credentials for the storage service is however implemented.

### A note about secure enclaves

`signal-server` implements a Contact Discovery Service without using a secure
enclave.  `signal-android` was modified to support this.

The use of server-side secure enclaves was
[introduced](https://signal.org/blog/secure-value-recovery/) to store
private data securely, so that the server operator would not have access to
the data or be able to hand it over, even when compelled by force of law.

The value of this has always been dubious.  Intel SGX keys use the NSA-tainted
P256 curve.  SGX has bugs and keys have leaked.  The false sense of security
has led to more data being stored server-side than was wise, including
recovery information that could give full access to an account.

People who, in spite of all this, really want to use a secure enclave for 
storing contact data, should revert commit
[3f10734](https://github.com/LPC-language/signal-android/commit/3f10734c3d1ce38d49459fc75c9bf934e1223e1d)
and use [CDSI](https://github.com/signalapp/ContactDiscoveryService-Icelake) to
implement the Contact Discovery Service.

## Installation

Although DGD runs on many platforms, some extension modules only compile on
Linux at present, so these instructions are for Linux.  It is assumed that
all git repositories are checked out side by side.

1.  Sign up for [Google Cloud](https://cloud.google.com).  A free account is
    needed for FCM notifications, which are also used during the Android client
    registration process.  It will take a day or so to activate your account,
    so start with this.
2.  Git checkout https://github.com/dworkin/dgd.git, and run `make
    DEFINES='-DEINDEX_TYPE="unsigned short" -DEINDEX_MAX=USHRT_MAX' install`
    in `dgd/src` to create `dgd/bin/dgd`.  Build requires a yacc-compatible
    parser generator such as bison or byacc.
3.  Git checkout https://github.com/dworkin/lpc-ext.git and run `make crypto`
    in `lpc-ext/src` to build the crypto extension module.  Requires OpenSSL
    development.
4.  Optionally, run `make jit` in `lpc-ext/src` to build the JIT compiler
    extension module.  Requires [clang](https://clang.llvm.org) to be installed
    both for building and for running the extension module.
5.  Git checkout https://github.com/LPC-language/libsignal.git and run
    `make` in `libsignal/src` to build the libsignal extension module.
    Requires OpenSSL development.
6.  Git checkout https://github.com/dworkin/cloud-server.git and
    https://github.com/LPC-language/signal-server.git and run
    `ln -s ../../../signal-server/src MsgServer` in `cloud-server/src/usr`.
7.  Git checkout https://github.com/LPC-language/signal-android.git and
    install Android Studio to build it later.
8.  Copy `signal-server/src/config/services.example` to
    `signal-server/src/config/services`, and change the domain names in that
    file and in `signal-android/app/build.gradle`.  Also change static IP
    numbers in `signal-android/app/static-ips.gradle`.
9.  Generate a self-signed certificate for `*.yourdomain.com`, store it as
    `signal-server/src/config/cert/server.{pem,key}` and add the certificate to
    `signal-android/app/src/main/res/raw/whisper.store` using an application
    like [Keystore Explorer](https://keystore-explorer.org/).
10. From https://console.firebase.google.com, create a Firebase project for
    `signal-server` using your activated Google Cloud account.  Add an Android
    app to the project.  Download `google-services.json` for the app and update
    `signal-android/app/src/main/res/values/firebase_messaging.xml`
    accordingly (leave the "default_web_client_id" line as it is).
11. Go to "Project Settings/Service accounts", generate a new private key for
    the Firebase Admin SDK and save the downloaded JSON data as
    `signal-server/src/config/fcm-key/service-account.json`.  It should look
    like this:

        {
          "type": "service_account",
          "project_id": "project-name",
          "private_key_id": "0123456789abcdef0123456789abcdef",
          "private_key": "-----BEGIN PRIVATE KEY-----\nAAAA\n-----END PRIVATE KEY-----\n",
          "client_email": "firebase-adminsdk-xyz@project-name.iam.gserviceaccount.com",
          "client_id": "01234567890123456789",
          "auth_uri": "https://accounts.google.com/o/oauth2/auth",
          "token_uri": "https://oauth2.googleapis.com/token",
          "auth_provider_x509_cert_url": "https://www.googleapis.com/oauth2/v1/certs",
          "client_x509_cert_url": "https://www.googleapis.com/robot/v1/metadata/x509/firebase-adminsdk-xyz%40project-name.iam.gserviceaccount.com",
          "universe_domain": "googleapis.com"
        }

12. Start the server with `dgd/bin/dgd signal-server/server.dgd`.  This will
    create `signal-server/src/config/ZKGROUP_SERVER_PUBLIC_PARAMS`.  Copy the
    contents to `android/defaultConfig/ZKGROUP_SERVER_PUBLIC_PARAMS` in
    `signal-android/app/build.gradle`.
13. `signal-server` listens on port 8443.  Listening on port 443 would require
    running the server as root and is not recommended, so redirect port 443 to
    8443 with a command like

        sudo iptables -t nat -A PREROUTING -d 192.168.0.1 -p tcp --dport 443 -j DNAT --to-destination 192.168.0.1:8443

14. Build the Android client.
15. Run the Android client.  Register with the server, using any verification
    code when asked (the server will not send one through SMS) and choose to
    skip and then disable a PIN code.  You should now be able to send
    end-to-end encrypted messages to other registered clients.

## Running the server

DGD was originally made for
[MUDs](https://en.wikipedia.org/wiki/Multi-user_dungeon), and some of the
architecture still reflects this.  With telnet, one can connect to a kind
of "shell" built into the server, giving low-level control to the user.  This
is required for development, to compile and upgrade objects inside a
running server.

To get started, you login as admin, a user with all permissions but a
restricted set of commands.  Create a new user, which will be able to use
an extended shell, including commands to compile, destruct and upgrade objects,
create a database snapshot, reboot the server, and even reboot into a new
version of DGD without taking down the server.  The example below shows how to
create a new user "dworkin":

    > telnet localhost 8023
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.

    Welcome to the Cloud Server.

    After login, the following commands are available:
     - users                see who is logged on
     - say, ', emote, :     communication
     - quit                 leave the mud

    login: admin
    Pick a new password:
    Retype new password:
    Password changed.
    # grant dworkin access
    # grant dworkin / full
    # quit

Commands available to the new user include:

-   `compile /path/to/object.c`: compile, or re-compile, an LPC source file
-   `destruct /path/to/object`: destruct an object
-   `upgrade /path/to/object.c`: recompile an object, and all other objects
    that inherit from it.  `upgrade -a` can be used to perform the upgrade
    atomically, so that nothing will change if any object failed to compile.
-   `snapshot`: create a database snapshot of the current state, `snapshot -f`
    to create a full snapshot which does not depend on the previous snapshot
-   `reboot`: create a snapshot of the current state and halt the server,
    permitting it to be restarted with the snapshot later
-   `hotboot`: create a snapshot and execute a new version of DGD directly,
    permitting DGD to be upgraded without downtime
-   `halt`: halt the server without creating a snapshot
-   `cd`, `pwd`, `ls`, `cp`, `rm`, `ed`: similar to Unix shell commands

Snapshots are created in `cloud-server/state`.  The most recent snapshot
is called `snapshot`, the previous snapshot is `snapshot.old`, and older
snapshots are not kept by DGD (but could be preserved by an external script).
DGD can be started with a snapshot by adding it to the command line, typically
`dgd/bin/dgd signal-server/server.dgd cloud-server/state/snapshot
cloud-server/state/snapshot.old`.
