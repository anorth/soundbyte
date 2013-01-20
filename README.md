scom
====
Example usage:

In one terminal, run
`cat gen.py | python run.py -s`.

In another terminal, run
`python run.py -l`. Turn on your speakers :P


Using AudioServer
================
AudioServer listens for TCP connections on a port (default 16000) then pipes raw mic data
to a connected client.

Run the app on your phone, then
`adb forward tcp:16000 tcp:16000`
` nc localhost 16000 | ./run.py -l -s`


Setup
=====

Install http://hathawaymix.org/Software/ReedSolomon/
And other stuff
