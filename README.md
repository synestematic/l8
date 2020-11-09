# L8proxy 

L8proxy is a simple tool that sits in the middle of a TCP client/server connection and allows the end-user to apply a time-delay to any of the different parts of the communication. 

### History

The default linux `tc` tool is only able to delay communication over entire network interface and therefore does not allow to single out specific point-of-failures in data flow.

``` 
tc qdisc add dev eth0 root netem delay 1000ms  
```

The granular control that `l8` provides is achieved by handling each socket using a different thread and means that delaying one end of the communication has no direct impact on the performance of the others.

### Data Flow Diagram:

``` 
..
     client    ===>   L8    --->   server
    software   <---  PROXY  <===  software

LEGEND:
===> recv socket
---> frwd socket

```
<br/>
