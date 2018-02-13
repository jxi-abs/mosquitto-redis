mosquitto-krb5
==============

*mosquitto-redis* is a project that contains both an authentication plugin for
the [mosquitto][] broker, and a library that can be injected into processes
using libmosquitto.

This library is released under the Simplified BSD License. For the full license
text, please see the `LICENSE` file.


auth-plugin
-----------

The authentication plugin for mosquitto is very limited, dealing only with basic
kerberos 5 (libkrb5) authentication. The restrictive plugin architecture in
mosquitto means there is no way to do ACL checking, TLS-PSK authentication
and/or authentication using other mechanisms. My intention is to either work
around this, get mosquitto to support multiple plugins, or integrate with
another plugin that has support for other backends (like jpmens'
[mosquitto-auth-plug][]).

In its current state, the plugin allows setting a keytab location (defaulting to
the system default) that contains the broker's key, belonging to principal
`mqtt/fqdn@DOMAIN`, for example `mqtt/test.mosquitto.org@MOSQUITTO.ORG`. It also
allows to set a `principal_format`, which is a printf-style format (only allows
`%s` and only allows it once) used to convert the given username into a
principal. The default pattern `%s` means user `mosq` in the `EXAMPLE.COM`
kerberos domain would require the principal `mosq@EXAMPLE.COM` to authenticate
to the broker.

This plugin introduces two configuration options:

 - `auth_opt_keytab`: The location of the keytab file
 - `auth_opt_principal_format`: The format string to transform a username into a
	   principal name.



Compilation
-----------

To compile this project you will need:

 - A C99-compatible C compiler
 - pkg-config
 - hiredis-dev
 - [mosquitto][]/libmosquitto
 - running redis with valid cache contain

The commands `make` or `make all` build both the auth-plugin and the
client-preload. The `clean` target will, as per usual, remove the output files.
Similarly, using the output file names as targets will build only that
particular module.

For testing purposes `make server` starts a mosquitto broker with extra
verbosity enabled, and using the `mosquitto.conf` also in this repository, it
presumes that a keytab `mqtt.keytab` has been placed in this directory,
containing the key(s) for the broker.

Similarly, `make client` verbosely publishes a message to the broker running on
localhost (typically corresponding to the FQDN `localhost.localdomain`) using
your own user name.

[mosquitto]: http://mosquitto.org
[mosquitto-auth-plug]: https://github.com/jpmens/mosquitto-auth-plug
