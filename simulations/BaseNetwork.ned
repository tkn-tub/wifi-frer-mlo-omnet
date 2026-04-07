package frer_mlo.simulations;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.linklayer.configurator.StreamRedundancyConfigurator;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedMultiCanvasVisualizer;
import inet.common.lifecycle.NodeStatus;
import inet.common.scenario.ScenarioManager;
import inet.node.ethernet.EthernetLink;

import frer_mlo.node.Ieee8021CbStation;
import frer_mlo.node.Ieee8021CbAccessPoint;

network BaseNetwork
{
    parameters:
        @figure[panel](type=panel; pos=600,0);
        @display("bgb=600,600");

    submodules:
        visualizer: IntegratedMultiCanvasVisualizer {
            parameters:
                @display("p=50,33");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=50,100");
        }
        configurator: Ipv4NetworkConfigurator {
            parameters:
                @display("p=50,167");
        }
        scenarioManager: ScenarioManager {
            parameters:
                @display("p=50,300");
        }
        sta1: Ieee8021CbStation {
            parameters:
                @display("p=300,350");
        }
        sta2: Ieee8021CbStation {
            parameters:
                @display("p=300,150");
        }
        ap: Ieee8021CbAccessPoint {
            parameters:
                @display("p=300,250");
        }

}