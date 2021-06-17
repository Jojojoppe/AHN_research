//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package wireless.simulations.static;

import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211DimensionalRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.environment.common.PhysicalEnvironment;
import wireless.StaticNode;

network Scenario{
    
    parameters:
    	@display("bgb=600,430");
    	
    	double beaconPeriod = default(0.1);
        double dataExchangeInterval = default(0.050);
        int nrPackets = default(600);
        int nrBytesPerPacket = default(1600);
        double mmwLoopTime = default(0.1);
        int txCount = default(1);
        int txDstCount = default(4);
        double txPacketLoss = default(0.00);
        double rxPacketLoss = default(0.00);
        bool retry = default(false);
    	
    submodules:
        radioMedium: Ieee80211DimensionalRadioMedium {
            @display("p=30,330");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=30,390");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=90,390");
        }

        node0: StaticNode {
            @display("p=356,289");
        }
        
        node1: StaticNode {
            @display("p=288,207");
        }

        node2: StaticNode {
            @display("p=215,129");
        }

        node3: StaticNode {
            @display("p=215,289");
        }

        node4: StaticNode {
            @display("p=356,129");
        }

        node5: StaticNode {
            @display("p=288,366");
        }

        node6: StaticNode {
            @display("p=288,52");
        }
}
