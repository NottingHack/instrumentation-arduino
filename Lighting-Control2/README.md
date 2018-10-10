# Nottingham Hackspace Automated Lighting Controller


## Input channel configuration
For each in put when the state is change as message is published to MQTT with the new state

`nh/li/CNCRoomController/I1/state OFF`

To program a non server change to of the output state use menu option 7
For each channel you set three things

**Mask:**
24 bits that represent which output channel will be effected by this input channel
1, this outputs state will be changed
0, this output state will not be changed
enter a 6 hex chars

**States:**
24 bits that represent which state the channel will be set to
1, this channel will be set ON
0, this channel will be set OFF
enter a 6 hex chars

**State-full:**
If enable then the activations will be tracked and every other activation of the input will result in the opposite `States` being set on the output channels

