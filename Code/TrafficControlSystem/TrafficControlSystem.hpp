#ifndef TRAFFICCONTROLSYSTEM_TRAFFICCONTROLSYSTEM_HPP
#define TRAFFICCONTROLSYSTEM_TRAFFICCONTROLSYSTEM_HPP

#include <queue>
#include <string>
#include <vector>
#include <memory>

#include "Mediator.hpp"
#include "TrafficStrategy/TrafficStrategy.hpp"
#include "TrafficSemaphore/TrafficSemaphore.hpp"
#include "PedestrianSemaphore/PedestrianSemaphore.hpp"
#include "Messages/Components/Cloud/QueueSendCloudTypes.hpp"
#include "CppWrapper/CppWrapper.hpp"
#include "CloudInterface/CloudInterface.hpp"
#include "Subscriber/DDSSubscriber.hpp"

#define DEFAULT_SWITCHING_TIME 5   //s

class I_TrafficStrategy;

//  Meyers Singleton, Mediator
class TrafficControlSystem: public Mediator
{
public:
    /*--- System Types ---------------------------------------------------------------------------------------------- */
    enum class SystemState {
        SET_UP,
        NORMAL,
        EMERGENCY,
        FAILURE
    };

private:
    typedef struct
     {
        PedestrianSemaphore* psem1;
        PedestrianSemaphore* psem2;
     } Crosswalk;

    typedef struct
    {
        std::vector<TrafficSemaphore*> activeTsem;
        std::vector<Crosswalk*> crosswalk;
        int time = DEFAULT_SWITCHING_TIME;
    } Configuration;

   enum class Components        // Used for component creation
    {
        INVALID,
        PEDESTRIAN_SEMAPHORE,
        TRAFFIC_SEMAPHORE,
        DDS_SUBSCRIBER
    };

    /* --- Attributes ----------------------------------------------------------------------------------------------- */
  //  std::string password;
    std::string username;

    std::vector<int> availableGPIOs; // ALL available pins at any given moment. Starts full.
    std::vector<int> usedGPIOs; // ALL used pins at any given moment. Starts empty.

    std::vector<std::unique_ptr<TrafficSemaphore>> TrafficSemVector;
    std::vector<std::unique_ptr<PedestrianSemaphore>> PedestrianSemVector;
    CloudInterface cloud;
    using  DDS_Subscriber = eprosima::fastdds::examples::emergencyMSG::DDSSubscriber;
    DDS_Subscriber ddsSubscriber;

    queue<tx_cloud::EmergencyContext> emergencies; // FIFO: Stores current EV on intersection info

    std::vector<std::unique_ptr<Crosswalk>> crosswalks; // Stores Intersection's Crosswalks

    std::vector<Configuration> configurations; // Stores Intersection's Configurations
    int current_config_idx;

    SystemState state;

    //static SwitchLightsData switchingData;
    // ------------------- Undirected Conflict Graph Logic ------------------------
    std::vector<std::vector<bool>> conflictGraph;  // Modified Graph Adjacency Matrix (O(1) access)
    // Graph Virtual Structure
    std::vector<int> vertices;

    using IntersectionElement = std::variant<       // for extensibility option to other signs
                            TrafficSemaphore*,
                            Crosswalk*
                            >;

    std::vector<IntersectionElement> elementByLocation;
    static int maxLocation; // stores the maximum Location/Destination in the configuration

    /* --- Private Constructor - Singleton -------------------------------------------------------------------------  */
    TrafficControlSystem();

    /* --- Registry Factory ----------------------------------------------------------------------------------------- */
    using ComponentCreator = std::function<int(const json&)>;
    std::unordered_map<Components, ComponentCreator> componentFactory;

    void initComponentFactory();

    /* --- Handling Strategy ---------------------------------------------------------------------------------------- */
    I_TrafficStrategy* TrafficStrategy;
    std::unordered_map<SystemState, std::unique_ptr<I_TrafficStrategy>> TrafficStrategies;

    void setStrategy();
    void initTrafficStrategies();

    /* --- System Signals - Stop System Execution ------------------------------------------------------------------- */
    static void initSystemSignals();
    static void SystemSignalsHandler(int signum);

    /* --- Helper Methods ------------------------------------------------------------------------------------------- */
    static Components isValidName (const std::string& name);
    int checkLocation (Components cp, int loc) const;
    int processPin (int pin);
    void setCrosswalks();

    static bool conflictTrajectory (const TrafficSemaphore& semA, const TrafficSemaphore& semB);
    static bool conflictTrajectory (const TrafficSemaphore& semA, const Crosswalk& crosswalk);

    void setUpGraphMatrix();
    static bool isBetween(int a, int b, int x);
    void backtrack(std::vector<int>& current, std::vector<int>& candidates);

public:

    /*  This data structure aims to be used only with Queue destined for
     * light switching.
     */
    typedef struct
    {
        std::vector<TrafficSemaphore*> ON_Tsem;
        std::vector<TrafficSemaphore*> OFF_Tsem;
        std::vector<Crosswalk*> ON_Crosswalk;
        std::vector<Crosswalk*> OFF_Crosswalk;
        double time;
    }SwitchLightsData;

    SwitchLightsData currentSwitchingData; // keeps track of the current information required to switch Configuration

   static std::atomic<bool> _shutdown_requested;

    /* --- Constructor/Destructor/Singleton ------------------------------------------------------------------------- */
   TrafficControlSystem(const TrafficControlSystem&)=delete; // delete copy constructor, prevent copies
   TrafficControlSystem& operator=(const TrafficControlSystem&) = delete;
   TrafficControlSystem(TrafficControlSystem&&) = delete;
   TrafficControlSystem& operator=(TrafficControlSystem&&) = delete;

    ~TrafficControlSystem()override;
    static TrafficControlSystem& getInstance();
    void start(); // Facade
    void startComponents() const;
    void waitStop();

    /* --- Mediator Interface --------------------------------------------------------------------------------------- */
    void notify (Component* sender, Event event) override;
    int createComponents (const std::shared_ptr<json>& data_file) override;

    /* --- Consumer Logic ------------------------------------------------------------------------------------------- */
    void consumer();

    /* --- System Handling ------------------------------------------------------------------------------------------ */
    void switch_state (SystemState next_state);

    void letPedestriansCross(const std::vector<Crosswalk*>& ChangeCrosswalksVec, bool emergency);
    void stopPedestriansCross(const std::vector<Crosswalk*>& ChangeCrosswalksVec, bool emergency);

    void letCarsMove(const std::vector<TrafficSemaphore*>& ChangeSemVec);
    void prepareToStopCars(const std::vector<TrafficSemaphore*>& ChangeSemVec);
    void stopCarsMove(const std::vector<TrafficSemaphore*>& ChangeSemVec);

   // void updateCloud (SwitchLightsData& data, bool isYellow);

    void updateSemaphoresCloud(TrafficSemaphore* sem, int light_state);
    void updateSemaphoresCloud(Crosswalk* cross, int light_state);
    void sendToCloud(CloudSendType message);

    /* --- System Evaluation ---------------------------------------------------------------------------------------- */
    void findConfigurations ();
    bool PSEM_Button_HasExtended(int location) const;

    /* --- Search/Organize Methods ---------------------------------------------------------------------------------- */
    template <typename T>
    void sortSemByLocation(std::vector<std::unique_ptr<T>>& vec);
    std::vector<Crosswalk*> searchCrosswalk(int location, int direction) const;
    std::vector<TrafficSemaphore*> searchTSEM(int location, int direction) const;

    void pushEmergency (tx_cloud::EmergencyContext info);
    void popEmergency ();
    tx_cloud::EmergencyContext getEmergency();
    //void sendEmergencyToCloud ();
    [[nodiscard]] int numEmergencies() const;

    SwitchLightsData organizeNextConfiguration(int config_idx_em=0);
    int EVneedChangeConfiguration ();

    SwitchLightsData systemWarning();

    void searchConfigurationForRFID(int location);
    /*--- Helper -----------------------------------------------------------------------------------------------------*/
    void stopCurrentTime();
    /*---Threading & Synchronization Resources------------------------------------------------------------------------*/
    CppWrapper::Queue<Event> eventQueue;

    CppWrapper::Thread tcsThread;
    static void* t_tcs(void* arg);

    CppWrapper::Queue<SwitchLightsData> switchLightQueue;

    CppWrapper::Thread switchLightThread;
    static void* t_switchLight(void* arg);

    CppWrapper::Timer timerSwitchLight;

    static CppWrapper::Mutex mutexShutdown;
    static CppWrapper::CondVar condShutdown;

};

#endif //TRAFFICCONTROLSYSTEM_TRAFFICCONTROLSYSTEM_HPP