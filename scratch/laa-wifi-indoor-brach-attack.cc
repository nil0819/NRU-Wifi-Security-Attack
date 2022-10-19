/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2015 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es> and Tom Henderson <tomh@tomh.org>
 */

//
//  From TR36.889-011 Study on LAA:
//  "Two operators deploy 4 small cells each in the single-floor building.
//  The small cells of each operator are equally spaced and centered along the
//  shorter dimension of the building. The distance between two closest nodes
//  from two operators is random. The set of small cells for both operators
//  is centered along the longer dimension of the building."
//
//   +--------------------120 m---------------------+
//   |                                              |
//   |                                              |
//   |                                              |
//   50m     x o      x o          x o      x o     |
//   |                                              |
//   |                                              |
//   |                                              |
//   +----------------------------------------------+
//
//  where 'x' and 'o' denote the small cell center points for the two operators.
//
//  In Wi-Fi, the 'x' and 'o' correspond to access points.
//
//  We model also N UEs (STAs) associated with each cell.  N defaults to 10.
//
//  The UEs (STAs) move around within the bounding box at a speed of 3 km/h.
//  In general, the program can be configured at run-time by passing

//  command-line arguments.  The command
//  ./waf --run "laa-wifi-indoor --help"
//  will display all of the available run-time help options, and in
//  particular, the command
//  ./waf --run "laa-wifi-indoor --PrintGlobals" should
//  display the following:
//
// Global values:
//     --ChecksumEnabled=[false]
//         A global switch to enable all checksums for all protocols
//     --RngRun=[1]
//         The run number used to modify the global seed
//     --RngSeed=[1]
//         The global seed of all rng streams
//     --SchedulerType=[ns3::MapScheduler]
//         The object class to use as the scheduler implementation
//     --SimulatorImplementationType=[ns3::DefaultSimulatorImpl]
//         The object class to use as the simulator implementation
//     --cellConfigA=[Wifi]
//         Lte or Wifi
//     --cellConfigB=[Wifi]
//         Lte or Wifi
//     --duration=[10]
//         Data transfer duration (seconds)
//     --pcapEnabled=[false]
//         Whether to enable pcap trace files for Wi-Fi
//     --transport=[Udp]
//         whether to use Udp or Tcp
//
//  The bottom five are specific to this example.
//
//  In addition, some other variables may be modified at compile-time.
//  For simplification, an IEEE 802.11n configuration is used with the
//  MinstrelWifi (with the 802.11a basic rates) is used.
//
//  The following sample output is provided.
//
//  When run with no arguments:
//
//  ./waf --run "laa-wifi-indoor"
//  Running simulation for 10 sec of data transfer; 50 sec overall
//  Number of cells per operator: 1; number of UEs per cell: 2
// Flow 1 (100.0.0.1 -> 100.0.0.2)
//   Tx Packets: 12500
//   Tx Bytes:   12850000
//   TxOffered:  10.28 Mbps
//   Rx Bytes:   12850000
//   Throughput: 10.0609 Mbps
//   Mean delay:  215.767 ms
//   Mean jitter:  0.277617 ms
//   Rx Packets: 12500
// Flow 2 (100.0.0.1 -> 100.0.0.3)
//   Tx Packets: 12500
//   Tx Bytes:   12850000
//   TxOffered:  10.28 Mbps
//   Rx Bytes:   6134076
//   Throughput: 4.80292 Mbps
//   Mean delay:  211.56 ms
//   Mean jitter:  0.140135 ms
//   Rx Packets: 5967
//
// In addition, the program outputs various statistics files from the
// available LTE and Wi-Fi traces, including, for Wi-Fi, some statistics
// patterned after the 'athstats' tool, and for LTE, the stats
// described in the LTE Module documentation, which can be found at
// https://www.nsnam.org/docs/models/html/lte-user.html#simulation-output
//
// These files are named:
//  athstats-ap_002_000
//  athstats-sta_003_000
//  DlMacStats.txt
//  DlPdcpStats.txt
//  DlRlcStats.txt
//  DlRsrpSinrStats.txt
//  DlRxPhyStats.txt
//  DlTxPhyStats.txt
//  UlInterferenceStats.txt
//  UlRxPhyStats.txt
//  UlSinrStats.txt
//  UlTxPhyStats.txt
//  etc.

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/scenario-helper.h>
#include <ns3/laa-wifi-coexistence-helper.h>
#include <ns3/lbt-access-manager.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LaaWifiIndoor");

// Global Values are used in place of command line arguments so that these
// values may be managed in the ns-3 ConfigStore system.
static ns3::GlobalValue g_duration ("duration", "Data transfer duration (seconds)",
                                    ns3::DoubleValue (2), ns3::MakeDoubleChecker<double> ());

/* Rashed Nov 26, 2021
* changing Cell A to LTE
*/
static ns3::GlobalValue g_cellConfigA ("cellConfigA", "Lte, Wifi, or Laa", ns3::EnumValue (LAA),
                                       ns3::MakeEnumChecker (WIFI, "Wifi", LTE, "Lte", LAA, "Laa"));
static ns3::GlobalValue g_cellConfigB ("cellConfigB", "Lte, Wifi, or Laa", ns3::EnumValue (WIFI),
                                       ns3::MakeEnumChecker (WIFI, "Wifi", LTE, "Lte", LAA, "Laa"));

static ns3::GlobalValue g_channelAccessManager ("ChannelAccessManager", "Default, DutyCycle, Lbt",
                                                ns3::EnumValue (Lbt),
                                                ns3::MakeEnumChecker (Default, "Default", DutyCycle,
                                                                      "DutyCycle", Lbt, "Lbt"));

static ns3::GlobalValue g_lbtTxop ("lbtTxop", "TxOp for LBT devices (ms)", ns3::DoubleValue (8.0),
                                   ns3::MakeDoubleChecker<double> (4.0, 20.0));

static ns3::GlobalValue g_useReservationSignal (
    "useReservationSignal",
    "Defines whether reservation signal will be used when used channel access manager at LTE eNb",
    ns3::BooleanValue (true), ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_laaEdThreshold ("laaEdThreshold",
                                          "CCA-ED threshold for channel access manager (dBm)",
                                          ns3::DoubleValue (-72.0),
                                          ns3::MakeDoubleChecker<double> (-100.0, -50.0));

static ns3::GlobalValue g_pcap ("pcapEnabled", "Whether to enable pcap trace files for Wi-Fi",
                                ns3::BooleanValue (false), ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_ascii ("asciiEnabled", "Whether to enable ascii trace files for Wi-Fi",
                                 ns3::BooleanValue (false), ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_transport ("transport", "whether to use 3GPP Ftp, Udp, or Tcp",
                                     ns3::EnumValue (UDP),
                                     ns3::MakeEnumChecker (FTP, "Ftp", UDP, "Udp", TCP, "Tcp"));

static ns3::GlobalValue g_lteDutyCycle ("lteDutyCycle", "Duty cycle value to be used for LTE",
                                        ns3::DoubleValue (1),
                                        ns3::MakeDoubleChecker<double> (0.0, 1.0));

/* Rashed Nov 26, 2021
* Change the bsSpacing Value to make the APs extremely closer
*/
static ns3::GlobalValue
    g_bsSpacing ("bsSpacing",
                 "Spacing (in meters) between the closest two base stations of different operators",
                 ns3::DoubleValue (4), ns3::MakeDoubleChecker<double> (1.0, 12.5));

static ns3::GlobalValue g_bsCornerPlacement (
    "bsCornerPlacement",
    "Rather than place base stations along axis according to TR36.889, place in corners instead",
    ns3::BooleanValue (false), ns3::MakeBooleanChecker ());

// Higher lambda means faster arrival rate; values [0.5, 1, 1.5, 2, 2.5]
// recommended
static ns3::GlobalValue g_ftpLambda ("ftpLambda", "Lambda value for FTP model 1 application",
                                     ns3::DoubleValue (0.5), ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_voiceEnabled ("voiceEnabled", "Whether to enable voice",
                                        ns3::BooleanValue (false), ns3::MakeBooleanChecker ());

static ns3::GlobalValue
    g_generateRem ("generateRem",
                   "if true, will generate a REM and then abort the simulation;"
                   "if false, will run the simulation normally (without generating any REM)",
                   ns3::BooleanValue (false), ns3::MakeBooleanChecker ());

static ns3::GlobalValue
    g_simTag ("simTag",
              "tag to be appended to output filenames to distinguish simulation campaigns",
              ns3::StringValue ("default"), ns3::MakeStringChecker ());

static ns3::GlobalValue g_outputDir ("outputDir", "directory where to store simulation results",
                                     ns3::StringValue ("./"), ns3::MakeStringChecker ());

static ns3::GlobalValue g_cwUpdateRule (
    "cwUpdateRule", "Rule that will be used to update contention window of LAA node",
    ns3::EnumValue (LbtAccessManager::NACKS_80_PERCENT),
    ns3::MakeEnumChecker (ns3::LbtAccessManager::ALL_NACKS, "all", ns3::LbtAccessManager::ANY_NACK,
                          "any", ns3::LbtAccessManager::NACKS_10_PERCENT, "nacks10",
                          ns3::LbtAccessManager::NACKS_80_PERCENT, "nacks80"));

static ns3::GlobalValue g_indoorLossModel ("indoorLossModel",
                                           "TypeId string of indoor propagation loss model",
                                           ns3::StringValue ("ns3::ItuInhPropagationLossModel"),
                                           ns3::MakeStringChecker ());

int
main (int argc, char *argv[])
{
  // Effectively disable ARP cache entries from timing out
  Config::SetDefault ("ns3::ArpCache::AliveTimeout", TimeValue (Seconds (10000)));
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Enable aggregation for AC_BE; see bug 2471 in tracker
  Config::SetDefault ("ns3::RegularWifiMac::BE_BlockAckThreshold", UintegerValue (2));

  // This program has two operators, and nominally 4 cells per operator
  // and 5 UEs per cell.  These variables can be tuned below for
  // e.g. debugging on a smaller scale scenario

  /* Rashed Nov 26, 2021
  * Making changes to the number of cells and UEs
  * This is a configurable code. Keeping the actual value commented so that I can change them back
  */
  //uint32_t numUePerCell = 5;
  //uint32_t numCells = 10;
  // uint32_t numUePerCell = 5;
  // uint32_t numCells = 4;

  // Some debugging settings not exposed as command line args are below

  // To disable application data flow (e.g. for visualizing mobility
  // in the visualizer), set below to true
  bool disableApps = false;

  // Extract global values into local variables.
  EnumValue enumValue;
  DoubleValue doubleValue;
  BooleanValue booleanValue;
  StringValue stringValue;
  GlobalValue::GetValueByName ("ChannelAccessManager", enumValue);
  enum Config_ChannelAccessManager channelAccessManager =
      (Config_ChannelAccessManager) enumValue.Get ();
  GlobalValue::GetValueByName ("cellConfigA", enumValue);
  enum Config_e cellConfigA = (Config_e) enumValue.Get ();
  GlobalValue::GetValueByName ("cellConfigB", enumValue);
  enum Config_e cellConfigB = (Config_e) enumValue.Get ();
  GlobalValue::GetValueByName ("lbtTxop", doubleValue);
  double lbtTxop = doubleValue.Get ();
  GlobalValue::GetValueByName ("laaEdThreshold", doubleValue);
  double laaEdThreshold = doubleValue.Get ();
  GlobalValue::GetValueByName ("duration", doubleValue);
  double duration = doubleValue.Get ();
  Time durationTime = Seconds (duration);
  GlobalValue::GetValueByName ("transport", enumValue);
  enum Transport_e transport = (Transport_e) enumValue.Get ();
  GlobalValue::GetValueByName ("lteDutyCycle", doubleValue);
  double lteDutyCycle = doubleValue.Get ();
  GlobalValue::GetValueByName ("bsSpacing", doubleValue);
  // double bsSpacing = doubleValue.Get ();
  // GlobalValue::GetValueByName ("bsCornerPlacement", booleanValue);
  // bool bsCornerPlacement = booleanValue.Get ();
  GlobalValue::GetValueByName ("generateRem", booleanValue);
  bool generateRem = booleanValue.Get ();
  GlobalValue::GetValueByName ("simTag", stringValue);
  std::string simTag = stringValue.Get ();
  GlobalValue::GetValueByName ("outputDir", stringValue);
  std::string outputDir = stringValue.Get ();
  GlobalValue::GetValueByName ("useReservationSignal", booleanValue);
  bool useReservationSignal = booleanValue.Get ();
  GlobalValue::GetValueByName ("cwUpdateRule", enumValue);
  enum LbtAccessManager::CWUpdateRule_t cwUpdateRule =
      (LbtAccessManager::CWUpdateRule_t) enumValue.Get ();

  GlobalValue::GetValueByName ("asciiEnabled", booleanValue);
  if (booleanValue.Get () == true)
    {
      PacketMetadata::Enable ();
    }
  GlobalValue::GetValueByName ("indoorLossModel", stringValue);
  std::string indoorLossModel = stringValue.Get ();

  Config::SetDefault ("ns3::LteChannelAccessManager::EnergyDetectionThreshold",
                      DoubleValue (laaEdThreshold));
  switch (channelAccessManager)
    {
    case Lbt:
      Config::SetDefault ("ns3::LaaWifiCoexistenceHelper::ChannelAccessManagerType",
                          StringValue ("ns3::LbtAccessManager"));
      Config::SetDefault ("ns3::LbtAccessManager::Txop", TimeValue (Seconds (lbtTxop / 1000.0)));
      Config::SetDefault ("ns3::LbtAccessManager::UseReservationSignal",
                          BooleanValue (useReservationSignal));
      Config::SetDefault ("ns3::LbtAccessManager::CwUpdateRule", EnumValue (cwUpdateRule));
      break;
    case DutyCycle:
      Config::SetDefault ("ns3::LaaWifiCoexistenceHelper::ChannelAccessManagerType",
                          StringValue ("ns3::DutyCycleAccessManager"));
      Config::SetDefault ("ns3::DutyCycleAccessManager::OnDuration", TimeValue (MilliSeconds (60)));
      Config::SetDefault ("ns3::DutyCycleAccessManager::OnStartTime", TimeValue (MilliSeconds (0)));
      Config::SetDefault ("ns3::DutyCycleAccessManager::DutyCyclePeriod",
                          TimeValue (MilliSeconds (80)));
      break;
    default:
      //default LTE channel access manager will be used, LTE always transmits
      break;
    }

  // When logging, use prefixes
  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);

  //
  // Topology setup phase
  //

  // Allocate 4 BS for nodes

  //std::cout << "Number of Cells" << numCells << std::endl;

  NodeContainer bsNodesA;
  bsNodesA.Create (3);
  NodeContainer bsNodesB;
  bsNodesB.Create (1); //numCells);

  //
  // Create bounding box 120m x 50m

  // BS mobility helper
  MobilityHelper mobilityBs;
  MobilityHelper mobilityBs_Mobile_Wifi;
  Ptr<RandomRectanglePositionAllocator> bs_allocator =
      CreateObject<RandomRectanglePositionAllocator> ();

  // Ptr<UniformRandomVariable> bs_xPos = CreateObject<UniformRandomVariable> ();
  // bs_xPos->SetAttribute ("Min", DoubleValue (16));
  // bs_xPos->SetAttribute ("Max", DoubleValue (72));
  // bs_allocator->SetX (bs_xPos);

  // Ptr<UniformRandomVariable> bs_yPos = CreateObject<UniformRandomVariable> ();
  // bs_yPos->SetAttribute ("Min", DoubleValue (0));
  // bs_yPos->SetAttribute ("Max", DoubleValue (60));
  // bs_allocator->SetY (bs_yPos);
  Ptr<ListPositionAllocator> initialAlloc = CreateObject<ListPositionAllocator> ();

  initialAlloc->Add (Vector (32, 28, 0));
  initialAlloc->Add (Vector (96, 28, 0));
  initialAlloc->Add (Vector (32, 84, 0));
  // initialAlloc->Add (Vector (1536, 1088, 0));
  // initialAlloc->Add (Vector (768, 1728, 0));
  // initialAlloc->Add (Vector (1280, 1728, 0));

  //bs_allocator->AssignStreams (1); // assign consistent stream number to r.vars.
  mobilityBs.SetPositionAllocator (initialAlloc);
  mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityBs.Install (bsNodesA);

  Ptr<ListPositionAllocator> initialAlloc_wifibs = CreateObject<ListPositionAllocator> ();
  initialAlloc_wifibs->Add (Vector (94, 28, 0));
  mobilityBs_Mobile_Wifi.SetPositionAllocator (initialAlloc_wifibs);
  mobilityBs_Mobile_Wifi.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityBs_Mobile_Wifi.Install (bsNodesB);

  // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (318), "MinY",
  //                                  DoubleValue (544), "DeltaX", DoubleValue (25), "LayoutType",
  //                                  StringValue ("RowFirst"));
  //mobilityBs.Install (bsNodesB);

  // mobilityBs_Mobile_Wifi.SetPositionAllocator (
  //     "ns3::GridPositionAllocator", "MinX", DoubleValue (16), "MinY", DoubleValue (16), "DeltaX",
  //     DoubleValue (25), "LayoutType", StringValue ("RowFirst"));

  // mobilityBs_Mobile_Wifi.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                            "Mode", StringValue ("Time"),
  //                            "Time", StringValue ("2s"),
  //                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
  //                            "Bounds", StringValue ("0|80|0|120"));
  //mobilityBs_Mobile_Wifi.Install (bsNodesB);

  // if (bsCornerPlacement == false)
  //   {
  //     /* Rashed Nov 26 2021
  //     * This i the BS placement code. Commenting the actual code so that I can change them back
  //     * I will be adding more deployment scenarios of the APs and BSs. 1X1 2X2 3X3 4X4 5X5 6X6
  //     */

  //     /* Rashed Nov 26 2021
  //     * My first case is 1X1. The AP and the BS would be at the center (58,25) and  (62,25)
  //     */
  //     // Place operator A's BS at coordinates (60,25)
  //     mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (58),
  //                                      "MinY", DoubleValue (25), "DeltaX", DoubleValue (15),
  //                                      "LayoutType", StringValue ("RowFirst"));
  //     mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  //     mobilityBs.Install (bsNodesA);
  //     // The offset between the base stations of operators A and B is governed
  //     // by the global value bsSpacing;
  //     // Place operator B's BS at coordinates (60 + bsSpacing ,25)
  //     mobilityBs.SetPositionAllocator (
  //         "ns3::GridPositionAllocator", "MinX", DoubleValue (58 + bsSpacing), "MinY",
  //         DoubleValue (25), "DeltaX", DoubleValue (25), "LayoutType", StringValue ("RowFirst"));
  //     mobilityBs.Install (bsNodesB);
  //     // // Place operator A's BS at coordinates (20,25), (45,25), (70,25), (95,25)
  //     // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
  //     //                                 "MinX", DoubleValue (20),
  //     //                                 "MinY", DoubleValue (25),
  //     //                                 "DeltaX", DoubleValue (25),
  //     //                                 "LayoutType", StringValue ("RowFirst"));
  //     // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  //     // mobilityBs.Install (bsNodesA);
  //     // // The offset between the base stations of operators A and B is governed
  //     // // by the global value bsSpacing;
  //     // // Place operator B's BS at coordinates (20 + bsSpacing ,25), (45,25), (70,25), (95,25)
  //     // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
  //     //                                 "MinX", DoubleValue (20 + bsSpacing),
  //     //                                 "MinY", DoubleValue (25),
  //     //                                 "DeltaX", DoubleValue (25),
  //     //                                 "LayoutType", StringValue ("RowFirst"));
  //     // mobilityBs.Install (bsNodesB);
  //   }
  // else
  //   {
  //     // Place BS at the corners (0,0), (120,0), (0,50), (120,50)
  //     mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (0),
  //                                      "MinY", DoubleValue (0), "GridWidth", UintegerValue (2),
  //                                      "DeltaX", DoubleValue (120), "DeltaY", DoubleValue (50),
  //                                      "LayoutType", StringValue ("RowFirst"));
  //     mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  //     mobilityBs.Install (bsNodesA);
  //     // place BS at the corners (1,1), (119,1), (1,49), (119,49)
  //     mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (30),
  //                                      "MinY", DoubleValue (38), "GridWidth", UintegerValue (2),
  //                                      "DeltaX", DoubleValue (118), "DeltaY", DoubleValue (48),
  //                                      "LayoutType", StringValue ("RowFirst"));
  //     mobilityBs.Install (bsNodesB);
  //   }
  /*Rashed Nov 29, 2021 
  */
  // Allocate UE for each cell
  NodeContainer ueNodesA;
  ueNodesA.Create (12); //numUePerCell * numCells);
  NodeContainer ueNodesB;
  ueNodesB.Create (4); //numUePerCell * numCells);

  // UE mobility helper
  MobilityHelper mobilityUe;
  // Ptr<RandomRectanglePositionAllocator> allocator =
  //     CreateObject<RandomRectanglePositionAllocator> ();
  Ptr<ListPositionAllocator> lte_initialAlloc = CreateObject<ListPositionAllocator> ();
  lte_initialAlloc->Add (Vector (32, 24, 0));
  lte_initialAlloc->Add (Vector (32, 32, 0));
  lte_initialAlloc->Add (Vector (28, 28, 0));
  lte_initialAlloc->Add (Vector (96, 24, 0));
  lte_initialAlloc->Add (Vector (96, 32, 0));
  lte_initialAlloc->Add (Vector (92, 28, 0));
  lte_initialAlloc->Add (Vector (32, 80, 0));
  lte_initialAlloc->Add (Vector (32, 88, 0));
  lte_initialAlloc->Add (Vector (28, 84, 0));
  // lte_initialAlloc->Add (Vector (96, 80, 0));
  // lte_initialAlloc->Add (Vector (96, 88, 0));
  // lte_initialAlloc->Add (Vector (92, 84, 0));
  // lte_initialAlloc->Add (Vector (1028, 1088, 0));
  // lte_initialAlloc->Add (Vector (1536, 1084, 0));
  // lte_initialAlloc->Add (Vector (1536, 1092, 0));
  // lte_initialAlloc->Add (Vector (1532, 1088, 0));
  // lte_initialAlloc->Add (Vector (1540, 1088, 0));
  // lte_initialAlloc->Add (Vector (768, 1724, 0));
  // lte_initialAlloc->Add (Vector (768, 1732, 0));
  // lte_initialAlloc->Add (Vector (764, 1728, 0));
  // lte_initialAlloc->Add (Vector (772, 1728, 0));
  // lte_initialAlloc->Add (Vector (1280, 1724, 0));
  // lte_initialAlloc->Add (Vector (1280, 1732, 0));
  // lte_initialAlloc->Add (Vector (1276, 1728, 0));
  // lte_initialAlloc->Add (Vector (1284, 1728, 0));

  Ptr<ListPositionAllocator> wifi_initialAlloc =
    CreateObject<ListPositionAllocator> ();

    wifi_initialAlloc->Add (Vector(98, 24, 0));
    wifi_initialAlloc->Add (Vector(98, 32, 0));
    wifi_initialAlloc->Add (Vector(90, 28, 0));
    // wifi_initialAlloc->Add (Vector(92, 28, 0));
  //   wifi_initialAlloc->Add (Vector(80,16,0));
  //   wifi_initialAlloc->Add (Vector(0,56,0));
  //   wifi_initialAlloc->Add (Vector(48,40,0));
  //   wifi_initialAlloc->Add (Vector(96,56,0));
  //   wifi_initialAlloc->Add (Vector(16,96,0));
  //   wifi_initialAlloc->Add (Vector(80,96,0));

  /*Rashed Nov 26 2021
  * I will make changes so that all the devices are close by
  */

  // Ptr<UniformRandomVariable> xPos = CreateObject<UniformRandomVariable> ();
  // xPos->SetAttribute ("Min", DoubleValue (16));
  // xPos->SetAttribute ("Max", DoubleValue (80));
  // allocator->SetX (xPos);
  // Ptr<UniformRandomVariable> yPos = CreateObject<UniformRandomVariable> ();
  // yPos->SetAttribute ("Min", DoubleValue (16));
  // yPos->SetAttribute ("Max", DoubleValue (120));
  // allocator->SetY (yPos);
  // Ptr<UniformRandomVariable> xPos = CreateObject<UniformRandomVariable> ();
  // xPos->SetAttribute ("Min", DoubleValue (0));
  // xPos->SetAttribute ("Max", DoubleValue (120));
  // allocator->SetX (xPos);
  // Ptr<UniformRandomVariable> yPos = CreateObject<UniformRandomVariable> ();
  // yPos->SetAttribute ("Min", DoubleValue (0));
  // yPos->SetAttribute ("Max", DoubleValue (50));
  // allocator->SetY (yPos);

  //allocator->AssignStreams (1); // assign consistent stream number to r.vars.
  mobilityUe.SetPositionAllocator (lte_initialAlloc);
  mobilityUe.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe.Install (ueNodesA);

  // My own Wifi Node setup
  // Ptr<UniformRandomVariable> wifi_xPos = CreateObject<UniformRandomVariable> ();
  // wifi_xPos->SetAttribute ("Min", DoubleValue (60));
  // wifi_xPos->SetAttribute ("Max", DoubleValue (64));
  // allocator->SetX (wifi_xPos);

  // Ptr<UniformRandomVariable> wifi_yPos = CreateObject<UniformRandomVariable> ();
  // wifi_yPos->SetAttribute ("Min", DoubleValue (24));
  // wifi_yPos->SetAttribute ("Max", DoubleValue (28));
  // allocator->SetX (wifi_yPos);

  //allocator->AssignStreams (1); // assign consistent stream number to r.vars.
  mobilityUe.SetPositionAllocator (wifi_initialAlloc);
  mobilityUe.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityUe.Install (ueNodesB);

  // REM settings tuned to get a nice figure for this specific scenario
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::OutputFile",
                      StringValue ("laa-wifi-indoor.rem"));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XMin", DoubleValue (0));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XMax", DoubleValue (120));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YMin", DoubleValue (0));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YMax", DoubleValue (120));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XRes", UintegerValue (120));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YRes", UintegerValue (120));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::Z", DoubleValue (1.5));

  std::ostringstream simulationParams;
  simulationParams << "";

  // Specify some physical layer parameters that will be used in scenario helper
  PhyParams phyParams;
  phyParams.m_bsTxGain = 0; // dB antenna gain //
  phyParams.m_bsRxGain = 0; // dB antenna gain
  phyParams.m_bsTxPower = 1; // dBm
  phyParams.m_bsNoiseFigure = 5; // dB
  phyParams.m_ueTxGain = 0; // dB antenna gain
  phyParams.m_ueRxGain = 0; // dB antenna gain
  phyParams.m_ueTxPower = 0; // dBm
  phyParams.m_ueNoiseFigure = 5; // dB

  ConfigureAndRunScenario (cellConfigA, cellConfigB, bsNodesA, bsNodesB, ueNodesA, ueNodesB,
                           phyParams, durationTime, transport, indoorLossModel, disableApps,
                           lteDutyCycle, generateRem, outputDir + "/laa_wifi_indoor_" + simTag,
                           simulationParams.str ());

  return 0;
}