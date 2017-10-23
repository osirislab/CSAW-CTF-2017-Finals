# Web solution

The KWS server will serialize JSON objects to pickle, then sign them with a preshared key.
The objects are then validated on the KWS instance, and unpicked before being stored.

Without being able to sign the pickle objects, attackers could not unpickle arbitrary objects,
only ones created from deserializing JSON, which is safe.

However, the same key is used to sign the share tokens. Using that endpoint, you can construct
a fake object store request with arbitrary pickle information.

The share token for object named `'STORE.pl.'+pickleData` can be passed to the action endpoint to
trigger arbitrary deserialization, running arbitrary code.

Here you can get a shell for the next part.

