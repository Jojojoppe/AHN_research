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

package wireless.simulations.straight;

import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211DimensionalRadioMedium;
import wireless.veins_inet.VeinsInetCar;
import wireless.veins_inet.VeinsInetManager;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import inet.environment.common.PhysicalEnvironment;
import org.car2x.veins.visualizer.roads.RoadsCanvasVisualizer;
import org.car2x.veins.visualizer.roads.RoadsOsgVisualizer;

network Scenario
{
    @display("bgb=600,430");
    submodules:
        radioMedium: Ieee80211DimensionalRadioMedium {
            @display("p=30,330");
        }
        manager: VeinsInetManager {
            @display("p=90,330");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=30,390");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=90,390");
        }
        roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=150,390");
        }
        roadsOsgVisualizer: RoadsOsgVisualizer {
            @display("p=150,330");
        }
        node[0]: VeinsInetCar {
            @display("p=210,330");
        }
}
