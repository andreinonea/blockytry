# Input handling

## Preamble

Module developer should be able to define a key input only once in their code
and call it a day. No .xml files, no other constructs. Whenever a key state is
polled in their code, through something like
```cpp
if (io::key_down (io::KEY_W)) // do something;
if (io::key_up (io::KEY_W)) // do something;
if (io::key_held (io::KEY_W)) // do something;
if (io::key_repeated (io::KEY_W)) // do something;
```
they should expect the framework to do the following things
- register the key to receive callbacks from the static input handler;
- be flexible to key bindings by the user;
- unbind both offenders when clashes occur, until user clarifies the situation.

What the second point means is that the user might assign 'W' to another action
or they might change the key for the action previously referenced by 'W' to 'E'.
This is nothing the developer should care about.

## Details

The framework sees such statement as the one above as a contract between itself
and the developer. The developer loosely says

> Please, keep my default key 'W' and bind it with **and** use whatever settings
> the user chooses.

This has the added benefit that keys can be restored to their default settings
at any time.

### Modes

As hinted above, there are multiple modes the user can interact with a key.
- `io::key_down`: user 