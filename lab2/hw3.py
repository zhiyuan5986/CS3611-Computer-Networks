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
        switch1 = self.addSwitch('s1')
        switch2 = self.addSwitch('s2')
        switch3 = self.addSwitch('s3')
        host1 = self.addHost('h1')
        host2 = self.addHost('h2')
        host3 = self.addHost('h3')
        host4 = self.addHost('h4')
        host5 = self.addHost('h5')
        host6 = self.addHost('h6')
        # add links between switchs
        self.addLink( switch1, switch2, bw=10, loss=0)
        self.addLink( switch1, switch3, bw=10, loss=0)
        self.addLink( switch2, switch3, bw=10, loss=0)
        # add links between switch and host
        self.addLink( switch1, host2)
        self.addLink( switch1, host4)
        self.addLink( switch2, host5)
        self.addLink( switch2, host6)
        self.addLink( switch3, host1)
        self.addLink( switch3, host3)
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