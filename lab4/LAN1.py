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
        switch1 = self.addSwitch('s1',ip="10.0.0.5")
        host1 = self.addHost('h1',ip="10.0.0.1")
        host2 = self.addHost('h2',ip="10.0.0.2")
        # add links between switch and host
        self.addLink( switch1, host1, bw=10,loss=0,delay='5ms')
        self.addLink( switch1, host2, bw=10,loss=0,delay='5ms')


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