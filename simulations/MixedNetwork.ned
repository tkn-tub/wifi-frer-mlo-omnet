package frer_mlo.simulations;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.linklayer.configurator.StreamRedundancyConfigurator;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.canvas.integrated.IntegratedMultiCanvasVisualizer;
import inet.common.lifecycle.NodeStatus;
import inet.common.scenario.ScenarioManager;
import inet.node.ethernet.EthernetLink;
import inet.node.tsn.TsnDevice;
import inet.node.tsn.TsnSwitch;

import frer_mlo.node.Ieee8021CbStation;
import frer_mlo.node.Ieee8021CbAccessPoint;
import mlo.node.MLOStation;

network MixedNetwork extends BaseNetwork
{
    parameters:
        int networkSize = default(0);
    
    submodules:
       
       slo_receiver_sta[int((networkSize - 2) / 2)]: MLOStation {
             parameters:
                umac.defaultInterface = "lmac" + string(intuniform(0, 1));         
       } 
       
       slo_sender_sta[int((networkSize - 2) / 2)]: MLOStation {
            parameters:
                umac.defaultInterface = "lmac" + string(intuniform(0, 1));     
                numApps = 1;
                app[0].typename = "UdpBasicApp";
                app[0].destAddresses = "slo_receiver_sta[" + string(index) + "]"; //  + MixedNetwork.network_size / 2
                app[0].packetName = "UDPData:" + string(index + 1);
                app[0].localPort = 5001;
                app[0].destPort = 5001;     
       }
        

 
}


