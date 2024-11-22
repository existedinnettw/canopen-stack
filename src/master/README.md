# master

The module implement CANopen master stack in software manner based on canopen-stack, hoping [extending](https://github.com/embedded-office/canopen-stack/issues/97#issuecomment-1096394108) its ability.
It not only contain NMT master but also include some abstraction outside spec of CANopen for ease of use.

## TODO

* rxpdo timeout
* It seem that `0x1010`, `0x1011` just store value in `CO_PARA`. It doesn't update OD value with those parameter.
