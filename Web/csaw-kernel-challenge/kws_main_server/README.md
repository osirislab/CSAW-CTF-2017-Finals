# Main Server

This flask server is the gateway to KWS. It provides an interface for users to interact with the KWS instances.

Each team will have a user, which will have a database entry. Each team should have an instance which also has a database
entry. (Currently hardcoded, but can be changed to read a config file etc).

Each instance will have a unique signing key, which should match the `/home/kws/box/key` file on the instance.

Teams can change their passwords using this interface.

Teams will be able to reset and reboot their instances incase something goes wrong. *(Not yet implmented)*

Teams will not be able to exploit any part of this server. It only helps them exploit their instances.
