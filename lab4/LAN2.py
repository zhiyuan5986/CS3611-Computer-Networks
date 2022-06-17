#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.cli import CLI
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel

class SingleSwitchTopo( Topo ):
    "Single switch connected to n hosts."
    def build( self ):
        switch2 = self.addSwitch('s2',ip="10.0.0.6")
        host3 = self.addHost('h3',ip="10.0.0.3")
        host4 = self.addHost('h4',ip="10.0.0.4")
        # add links between switch and host
        self.addLink( switch2, host3, bw=10,loss=0,delay='5ms')
        self.addLink( switch2, host4, bw=10,loss=0,delay='5ms')


def perfTest():
    "Create network and run simple performance test"
    topo = SingleSwitchTopo()
    net = Mininet( topo=topo,
	           link=TCLink )
    net.start()

    CLI(net)

    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    perfTest()