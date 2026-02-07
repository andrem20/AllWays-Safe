#include "TrafficControlSystem.hpp"

#include <cerrno>       // Error codes in: asm-generic/errno.h AND errno-base.h
#include <iostream>
#include <algorithm>
#include <atomic>
#include <unordered_set>
#include <csignal>
#include <variant>
#include <type_traits>

#define YELLOW_DURATION 2 // seconds
#define START_UP_CONFIG_DURATION 10 // seconds
#define FIRE_TIMER_IMMEDIATELY 1e-9

#define USE_CLOUD

int TrafficControlSystem::maxLocation = 0;

std::atomic<bool> TrafficControlSystem::_shutdown_requested{false};
CppWrapper::Mutex TrafficControlSystem::mutexShutdown;
CppWrapper::CondVar TrafficControlSystem::condShutdown(mutexShutdown);

TrafficControlSystem& TrafficControlSystem::getInstance()
{
    static TrafficControlSystem instance;
    return instance;
}

TrafficControlSystem::TrafficControlSystem():
    username("raspMari.local"),
    cloud("http://192.168.1.185:3000", username, "tmc1", this, _shutdown_requested),
    tcsThread(t_tcs),
    switchLightThread(t_switchLight),
    ddsSubscriber(_shutdown_requested, 0, "EmergencyAlert", this)
{
    state = SystemState::SET_UP;
    current_config_idx = 0;
    availableGPIOs = {1, 2, 3, 4, 5, 6, 7, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};//22 total

    initComponentFactory();
    initTrafficStrategies();
    initSystemSignals();
}

TrafficControlSystem::~TrafficControlSystem()
{
    std::cerr<<"TCS destroyed\n";
}

/* Initialize some system requirements
 *  Facade
 */
void TrafficControlSystem::start()
{
    // Start threads
    tcsThread.run(this);
    switchLightThread.run(this);

    cloud.cloudStart();
    ddsSubscriber.start();

    switch_state(state);
}

void TrafficControlSystem::startComponents() const
{
    for (auto& psem: PedestrianSemVector)
        psem->start();
}

void TrafficControlSystem::waitStop()
{
    switchLightQueue.interrupt();
    eventQueue.interrupt();

    ddsSubscriber.stop();
    cloud.stop();

    for (auto& psem: PedestrianSemVector)
        psem->stop();

    switchLightThread.join();
    tcsThread.join();
}

void TrafficControlSystem::initComponentFactory()
{
    /* Define lambdas to assign dynamic creation
     *  'this' keyword confers the knowledge of the current class instantiation to the function: allows the lambda
     *      to access member variables and member functions of the class
     *   since lambdas are not member functions: they are objects which behave like functions
     */
    componentFactory[Components::PEDESTRIAN_SEMAPHORE] =
     [this](const json& data) -> int
     {
         const std::string validationData [] = {"location", "gpio_red", "gpio_green", "hasButton",
             "hasCardReader", "hasBuzzer"};

         for (const auto &i : validationData)
         {
             // Data is expected to come as an integer
             if (!data.contains(i) || !data[i].is_number_integer())
                 throw::std::runtime_error("PSEM: " + i + " field not configured\n");
         }

         int loc = data["location"];
         if (checkLocation(Components::PEDESTRIAN_SEMAPHORE, loc) < 0)
             throw::std::runtime_error("PSEM: location invalid\n");

         int gpio_red = data["gpio_red"];
         int gpio_green = data["gpio_green"];
         if (processPin(gpio_red) < 0 || processPin(gpio_green) < 0)
             throw::std::runtime_error("PSEM: GPIO red/green exist\n");

         PedestrianFeatures feature = {};
         int threshold = 0;
         int gpio_button = 0;

         if (data["hasButton"]==1)
         {
             if (!data.contains("buttonThreshold") || !data.contains("gpio_button"))
                 throw::std::runtime_error("PSEM: GPIO button config not exist\n");

             threshold = data["buttonThreshold"];
             gpio_button = data["gpio_button"];
             if (processPin(gpio_button) < 0)
                 throw::std::runtime_error("PSEM: GPIO button exist\n");

             feature |= PedestrianFeatures::Button;
         }

         if (data["hasCardReader"]==1)
             feature |= PedestrianFeatures::CardReader;

         if (data["hasBuzzer"]==1)
             feature |= PedestrianFeatures::Buzzer;

         PedestrianSemVector.push_back(
         std::make_unique<PedestrianSemaphore>(
             this,
             _shutdown_requested,
             loc,
             feature,
             gpio_red,
             gpio_green,
             gpio_button,
             threshold

         ));

         return 0;
     };

    componentFactory[Components::TRAFFIC_SEMAPHORE]=
        [this](const json& data)
        {
            const std::string validationData [] = {"location", "destinations", "gpio_red", "gpio_green", "gpio_yellow"};

            for (const auto &i : validationData)
            {
                // Data is expected to come as an integer
                if (!data.contains(i) /*|| !data[i].is_number_integer(i)*/)
                    throw::std::runtime_error("TSEM: " + i + " field not configured\n");
            }

            int loc = data["location"];
            if (checkLocation(Components::TRAFFIC_SEMAPHORE, loc) < 0)
                throw::std::runtime_error("TSEM: location invalid\n");
            if (loc > maxLocation) maxLocation = loc;

            const int gpio_red = data["gpio_red"];
            const int gpio_green = data["gpio_green"];
            const int gpio_yellow = data["gpio_yellow"];

        /*    if (processPin(gpio_red) < 0 || processPin(gpio_green) < 0 || processPin(gpio_yellow) < 0)
                throw::std::runtime_error("TSEM: GPIO red/green/yellow exist\n");*/
            if (processPin(gpio_yellow) < 0)
                throw::std::runtime_error("TSEM: GPIO yellow\n"+std::to_string(loc));
            if (processPin(gpio_green) < 0)
                throw::std::runtime_error("TSEM: GPIO green\n"+std::to_string(loc));

            if (processPin(gpio_red) < 0)
                throw::std::runtime_error("TSEM: GPIO red\n"+std::to_string(loc));


            auto vec = data["destinations"].get<std::vector<int>>();
            std::unordered_set<int> destinations(vec.begin(), vec.end());

            for (const auto destination : destinations)
                if (destination > maxLocation) maxLocation = destination;

            TrafficSemVector.push_back(
                std::make_unique<TrafficSemaphore>(
                this,
                loc,
                destinations,
                gpio_red,
                gpio_green,
                gpio_yellow
                ));
            return 0;
        };
}

void TrafficControlSystem::initTrafficStrategies()
{
    TrafficStrategies[SystemState::SET_UP] = std::make_unique<StrategySetUp>();
    TrafficStrategies[SystemState::NORMAL] = std::make_unique<StrategyNormal>();
    TrafficStrategies[SystemState::EMERGENCY] = std::make_unique<StrategyEmergency>();
    TrafficStrategies[SystemState::FAILURE] = std::make_unique<StrategyFailure>();
}

// Inits system signals to stop the system execution
void TrafficControlSystem::initSystemSignals()
{
    _shutdown_requested.store(false);

    if (signal(SIGINT, SystemSignalsHandler) == SIG_ERR ||
        signal(SIGTERM, SystemSignalsHandler) == SIG_ERR ||
        signal(SIGHUP, SystemSignalsHandler))
        throw::std::runtime_error("TCS: initSystemSignals");
}

void TrafficControlSystem::SystemSignalsHandler(const int signum)
{
    //std::cout << "\nSignal " << signum << " received, stopping Traffic Control System..." << std::endl;
    _shutdown_requested.store(true);
    CppWrapper::LockGuard lock (mutexShutdown);
    condShutdown.condBroadcast();
}

void TrafficControlSystem::setStrategy()
{
    TrafficStrategy = TrafficStrategies[state].get();
}

void TrafficControlSystem::notify (Component* sender, Event event)
{
    std::cout << "TrafficSystem this: " << this << std::endl;
    eventQueue.send(std::move(event));
}

/*
 * Checks if the received json file is appropriate for Component Creation.
 * This is done by searching the string, from the beginning (arg: 0), for the specified substring
 */
TrafficControlSystem::Components TrafficControlSystem::isValidName (const std::string& s)
{
    Components result = Components::INVALID;
    if (s.size() >= 3)
    {
        if (!s.rfind("PS", 0))
            result = Components::PEDESTRIAN_SEMAPHORE;
        else if (!s.rfind("TS", 0))
            result = Components::TRAFFIC_SEMAPHORE;

        // Validate if the rest of the Semaphore is a valid input
        for (size_t i = 2; i < s.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(s[i])))
            {
                result = Components::INVALID;
                break;
            }
        }
    }
    return result;
}

// Check if Location was already attributed; Location must not be the same between (TSEMs), (PSEMs) and (TSEMs and PSEMs)
int TrafficControlSystem:: checkLocation (const Components cp, const int loc) const
{
    switch (cp)
    {
        case Components::PEDESTRIAN_SEMAPHORE:
            for (const auto& p : PedestrianSemVector) {
                if (p->getLocation() == loc)
                    return -EEXIST;             // If yes, return
            }
        break;
        case Components::TRAFFIC_SEMAPHORE:
            for (const auto& p : TrafficSemVector) {
                if (p->getLocation() == loc)
                    return -EEXIST;             // If yes, return
            }
            for (const auto& p : PedestrianSemVector) {
                if (p->getLocation() == loc)
                    return -EEXIST;             // If yes, return
            }
        break;
        default: return -EPERM;
    }
    return 0;
}

// Validate if pin is already in use or not, if not it removes it from the available pin vector and
// inserts it into the used pin vector
int TrafficControlSystem::processPin (const int pin)
{
    for (const auto p : availableGPIOs)
    {
        if (p == pin) // Pin is valid and not in use
        {
            std::erase(availableGPIOs, pin);
            usedGPIOs.push_back(pin);
            return 0;
        }
    }
    return -EEXIST;
}

int TrafficControlSystem::createComponents (const std::shared_ptr<json>& data_file)
{
    Components current_comp;

    for (auto& data: *data_file)
    {
        if (!data.contains("name") || !data["name"].is_string())
            return -EINVAL;

        const std::string name = data["name"];
        current_comp = isValidName(name);

        if (current_comp != Components::INVALID)
        {
            if (const int ret = componentFactory[current_comp] (data); ret < 0)
            {
                std::cout <<("Component '" + name + "' has repeated location\n");
                return ret;
            } // else, everything was properly configured
        }else
        {
            std::cout << ("Component '" + name + "' does not exist\n");
            return -EINVAL;
        }
    }

    switch (current_comp)
    {
        case Components::PEDESTRIAN_SEMAPHORE:
            sortSemByLocation(PedestrianSemVector);
            setCrosswalks();
            break;
        case Components::TRAFFIC_SEMAPHORE:
            sortSemByLocation(TrafficSemVector);
            break;
        default: break;
    }

    return 0;   // Success, all input was right and info was properly set
}

void TrafficControlSystem::switch_state (const SystemState next_state)
{
    state = next_state;
    setStrategy();

    this->notify(nullptr, InternalEvent::NEW_STATE_ENTERED);
}

template <typename T>
void TrafficControlSystem::sortSemByLocation(std::vector<std::unique_ptr<T>>& vec)
{
    std::sort(
        vec.begin(),        // Start
        vec.end(),          // End
        [](const std::unique_ptr<T>& a, const std::unique_ptr<T>& b)    // Sorting criteria
        {
            return a->getLocation() < b->getLocation();
            // returns true(1) if first arg is ordered before than the second
        }
    );
}

void TrafficControlSystem::letPedestriansCross(const std::vector<Crosswalk*>& ChangeCrosswalksVec, const bool emergency)
{
    if (!ChangeCrosswalksVec.empty())
    {
        for (const auto& crosswalk: ChangeCrosswalksVec)
        {
            crosswalk->psem1->switch_nextLight(Semaphore::TrafficColour::GREEN, emergency);
            crosswalk->psem2->switch_nextLight(Semaphore::TrafficColour::GREEN, emergency);

#ifdef USE_CLOUD
            updateSemaphoresCloud(crosswalk, static_cast<int>(Semaphore::TrafficColour::GREEN));
#endif
        }
    }
}

void TrafficControlSystem::stopPedestriansCross(const std::vector<Crosswalk*>& ChangeCrosswalksVec, const bool emergency)
{
    if (!ChangeCrosswalksVec.empty())
    {
        for (const auto& crosswalk: ChangeCrosswalksVec)
        {
            crosswalk->psem1->switch_nextLight(Semaphore::TrafficColour::RED, emergency);
            crosswalk->psem2->switch_nextLight(Semaphore::TrafficColour::RED, emergency);
#ifdef USE_CLOUD
            updateSemaphoresCloud(crosswalk, static_cast<int>(Semaphore::TrafficColour::RED));
#endif
        }
    }
}

void TrafficControlSystem::letCarsMove(const std::vector<TrafficSemaphore*>& ChangeSemVec)
{
    if (!ChangeSemVec.empty())
    {
        for (const auto& tsem: ChangeSemVec)
        {
            tsem->switch_nextLight(Semaphore::TrafficColour::GREEN, false);

#ifdef USE_CLOUD
            updateSemaphoresCloud(tsem, static_cast<int>(Semaphore::TrafficColour::GREEN));
#endif
        }
    }
}

void TrafficControlSystem::prepareToStopCars(const std::vector<TrafficSemaphore*>& ChangeSemVec)
{
    if (!ChangeSemVec.empty())
    {
        for (const auto& tsem: ChangeSemVec)
        {
            tsem->switch_nextLight(Semaphore::TrafficColour::YELLOW, false);

#ifdef USE_CLOUD
            updateSemaphoresCloud(tsem, static_cast<int>(Semaphore::TrafficColour::YELLOW));
#endif
        }
    }
}

/* Stops the movement of the cars acting on the TSEMs
 *  -> Evaluates what is the next configuration;
 *  -> Check if there are common semaphores on between the current configuration and the next configuration;
 *  -> Turn off (YELLOW, then, RED) the semaphores which require that.
 */
void TrafficControlSystem::stopCarsMove(const std::vector<TrafficSemaphore*>& ChangeSemVec)
{
    if (!ChangeSemVec.empty())
    {
        for (auto &tsem: ChangeSemVec)
        {
            tsem->switch_nextLight(Semaphore::TrafficColour::RED, false);
#ifdef USE_CLOUD
            updateSemaphoresCloud(tsem, static_cast<int>(Semaphore::TrafficColour::RED));
#endif
        }
        std::cerr << "[stopCarsMove] switched cars red \n";
    }
}

// returns true if time extension has already been required or error; false otherwise
bool TrafficControlSystem::PSEM_Button_HasExtended(const int location) const
{
    auto& element = elementByLocation[location];

    if (auto p_crosswalk = std::get_if<Crosswalk*>(&element))
    {
        auto& crosswalk = *p_crosswalk;

        return (crosswalk->psem1->getButtonEventCounter() > 1 | crosswalk->psem2->getButtonEventCounter() > 1);
    }

    return true;
}

// Checks if there is a collision between Traffic Semaphores' trajectories
bool TrafficControlSystem::conflictTrajectory(const TrafficSemaphore& semA,
                        const TrafficSemaphore& semB)
{
    const int La = semA.getLocation();
    const int Lb = semB.getLocation();

    for (const int Da : semA.getDirection()) {
        for (const int Db : semB.getDirection()) {

            // same direction = immediate conflict
            if (Da == Db)
                return true;

            // Check if trajectories cross (circle approach)
            // For each variable,
            //      if both are true, means the other trajectory is completely inside the first, so no problem.
            //      if both are false, means the other trajectory is completely outside the first, so no problem
            //      When they are different => there is a transition from inside to outside (vice versa) => problem
            const bool A_cross_B =
                isBetween(La, Da, Lb) ^ isBetween(La, Da, Db);

            const bool B_cross_A =
                isBetween(Lb, Db, La) ^ isBetween(Lb, Db, Da);

            if (A_cross_B || B_cross_A)
                return true;
        }
    }
    return false;
}

// Checks if there is a common direction between the Traffic Semaphore and the Crosswalk
bool TrafficControlSystem::conflictTrajectory (const TrafficSemaphore& semA, const Crosswalk& crosswalk)
{
    const int min = crosswalk.psem1->getLocation();
    const int max = crosswalk.psem2->getLocation();

    // Conflict from coming direction (aka Location)
    if (semA.getLocation() > min && semA.getLocation() < max)
        return true;

    // Conflict from heading direction
     for (const auto a : semA.getDirection()){
         if (a > min && a < max)
            return true;        // The crosswalk contains that direction
     }
     return false;
}

/*  Matrix indexes are identified by the Location attribute (Location are contiguous)
 *      - Item in conflictGraph[i][j] is
 *          - true: if there is conflict
 *          - false: if there isn't conflict
 */
void TrafficControlSystem::setUpGraphMatrix(){ // O(T²) + O(T*P)
    // Traffic Semaphores
    for (size_t i = 0; i < TrafficSemVector.size(); ++i)
    {
        for (size_t j = i + 1; j < TrafficSemVector.size(); ++j) {
            if (conflictTrajectory(*TrafficSemVector[i], *TrafficSemVector[j])) {
                conflictGraph[TrafficSemVector[i]->getLocation()][TrafficSemVector[j]->getLocation()]
                = conflictGraph[TrafficSemVector[j]->getLocation()][TrafficSemVector[i]->getLocation()]
                            = true; // Undirected Conflict Graph
            }
        }
        elementByLocation[TrafficSemVector[i]->getLocation()] = TrafficSemVector[i].get();
    }
    // Crosswalks/ Pedestrian Semaphores
    for (auto & crosswalk : crosswalks)
    {
        for (auto & j : TrafficSemVector)
        {
            if (conflictTrajectory(*j, *crosswalk))
            {
                conflictGraph[j->getLocation()][crosswalk->psem1->getLocation()]
                = conflictGraph[j->getLocation()][crosswalk->psem2->getLocation()]
                = conflictGraph[crosswalk->psem1->getLocation()][j->getLocation()]
                = conflictGraph[crosswalk->psem2->getLocation()][j->getLocation()]
                = true; // Undirected Conflict Graph
            }
        }
        elementByLocation[crosswalk->psem1->getLocation()] = crosswalk.get();
        elementByLocation[crosswalk->psem2->getLocation()] = crosswalk.get();
    }
}

/* Considers a Circular Approach to return whether trajectories intersect or not
 * Args:
 *  a and b shall be Location and Destination of a single semaphore.
 *  x is the location or destination of the other
 */
bool TrafficControlSystem::isBetween(const int a, const int b, const int x) {
    if (a < b)
        return x > a && x < b;
    return x > a || x < b;
}

// Apply backtracking w/ pruning Algorithm
/*      vector current holds the current locations to insert on the same configuration
 *      vector candidates holds all the locations, initially
 */
void TrafficControlSystem::backtrack(std::vector<int>& current, std::vector<int>& candidates)
{
    if (candidates.empty()) {       // Last iteration of each configuration
        if (!current.empty()) {
            Configuration cfg;
            for (const int loc : current)
            {
                const IntersectionElement& c = elementByLocation[loc];

                std::visit([&](auto&& obj) {
                    using T = std::decay_t<decltype(obj)>;

                    if constexpr (std::is_same_v<T, TrafficSemaphore*>) {
                        cfg.activeTsem.push_back(obj);
                    }
                    else if constexpr (std::is_same_v<T, Crosswalk*>) {
                        if (std::find(cfg.crosswalk.begin(),
                                      cfg.crosswalk.end(),
                                      obj) == cfg.crosswalk.end())
                            cfg.crosswalk.push_back(obj);
                    }
                }, c);
            }

            // Check if it is a maximal independent set - local validation:
            //      if any of the other locations (vertices) are compatible with this config
            bool maximal = true;
            for (int v : vertices) {
                if (std::find(current.begin(), current.end(), v) == current.end()) {
                    bool ok = true;
                    for (int u : current)
                        if (conflictGraph[v][u]) {
                            ok = false;
                            break;
                        }
                    if (ok) {
                        maximal = false;
                        break;
                    }
                }
            }

            if (maximal)
                configurations.push_back(cfg);
        }
        return;
    }

    while (!candidates.empty()) {
        int v = candidates.back();
        candidates.pop_back();

        // New subset of compatible candidates - next recursive iteration
        std::vector<int> newCandidates;
        for (int u : candidates) {
            if (!conflictGraph[v][u])           // pruning
                newCandidates.push_back(u);
        }

        current.push_back(v);
        backtrack(current, newCandidates);
        current.pop_back();
    }
}

void TrafficControlSystem::findConfigurations()
{
    size_t totalSize = maxLocation + 1;
    conflictGraph.resize(totalSize);
    for (auto &row : conflictGraph)
        row.resize(totalSize, false);

    elementByLocation.resize(totalSize);

    setUpGraphMatrix();

    std::vector<int> current;

    for (auto& tsem : TrafficSemVector)
        vertices.push_back(tsem->getLocation());

    for (auto& cross : crosswalks) {
        vertices.push_back(cross->psem1->getLocation());
        vertices.push_back(cross->psem2->getLocation());
    }
    std::vector<int> candidates = vertices;

    std::sort(candidates.begin(), candidates.end());

    backtrack(current, candidates);
}


/* Sets crosswalks based on each Pedestrian Semaphore Pair
 *  - Supposes that the Pedestrian Semaphore vector is already ordered,
 *      and that Pedestrian Semaphores were correctly created, so they are an even number
 */
void TrafficControlSystem::setCrosswalks ()
{
    for (size_t i = 0; i < PedestrianSemVector.size(); i += 2)
        crosswalks.push_back(std::make_unique<Crosswalk>(Crosswalk{
            PedestrianSemVector[i].get(),
            PedestrianSemVector[i+1].get()
        }));
}

// Used to know if the emergency vehicle is going to cross over any crosswalk
std::vector<TrafficControlSystem::Crosswalk*> TrafficControlSystem::searchCrosswalk
    (const int location, const int direction) const
{
    std::vector<Crosswalk*> return_sem;
    for (const auto& crosswalk: configurations[current_config_idx].crosswalk)
    {
        if ((crosswalk->psem1->getLocation() < location && crosswalk->psem2->getLocation() > location) |
            (crosswalk->psem1->getLocation() < direction && crosswalk->psem2->getLocation() > direction))
            return_sem.push_back(crosswalk);
    }
    return return_sem;
}

// Used to know the semaphore where the emergency vehicle is coming from
// Used to know the currently activated semaphores whose directions cross/collide with the EV direction
// Searches TSEMs by location and direction; this is useful for emergency vehicle interaction
std::vector<TrafficSemaphore*> TrafficControlSystem::searchTSEM(const int location, const int direction) const
{
    std::vector<TrafficSemaphore*> return_sem;

    for (const auto& tsem: configurations[current_config_idx].activeTsem)
    {
        // Search Location
        if (tsem->getLocation() == location)
            return_sem.push_back(tsem);

        // Search Direction
        for (const auto& dir: tsem->getDirection())
            if (dir <= direction) // Direction cross or collide
                return_sem.push_back(tsem);
    }
    return return_sem;
}

// Stores Emergency and Sends it to Cloud
void TrafficControlSystem::pushEmergency (tx_cloud::EmergencyContext info)
{
    emergencies.push(std::move(info));
}

void TrafficControlSystem::popEmergency ()
{
    if (!emergencies.empty())
        emergencies.pop();
}

tx_cloud::EmergencyContext TrafficControlSystem::getEmergency()
{
    return emergencies.front();
}

/*
void TrafficControlSystem::sendEmergencyToCloud()
{
    cloud.cloudSendQueue.send(emergencies.front());
}
*/
int TrafficControlSystem:: numEmergencies() const
{
    return static_cast<int>(emergencies.size());
}

/*
 * Must be called by Strategy when switching timer ends
 */
TrafficControlSystem::SwitchLightsData TrafficControlSystem::organizeNextConfiguration(int config_idx_em)
{
    int next_idx;
    if (state == SystemState::EMERGENCY)
        next_idx = config_idx_em;
    else     // Find next configuration index
        next_idx = (current_config_idx + 1) % static_cast<int>(configurations.size());

    // Clear switching Data
    SwitchLightsData switchingData = {};

    auto& current = configurations[current_config_idx];
    const auto& next = configurations[next_idx];

    // ---- TSEMs ----
    std::unordered_set<int> nextTsemLocations;
    for (const auto& tsem : next.activeTsem)
        nextTsemLocations.insert(tsem->getLocation());

    for (const auto& tsem : current.activeTsem)
        if (!nextTsemLocations.contains(tsem->getLocation()))
            switchingData.OFF_Tsem.push_back(tsem);                 // Before going RED, they go YELLOW

    // ---- Crosswalks ----
    for (const auto& cw : current.crosswalk)
    {
        bool turn_off = true;
        for (const auto& next_cw : next.crosswalk)
        {
            if (cw == next_cw)
            {
                turn_off = false;
                break;
            }
        }
        if (turn_off)
            switchingData.OFF_Crosswalk.push_back(cw);

        cw->psem1->resetButtonEventCounter();
        cw->psem2->resetButtonEventCounter();
    }

    // Define ON states
    switchingData.ON_Tsem = next.activeTsem;
    switchingData.ON_Crosswalk = next.crosswalk;

    if (state == SystemState::NORMAL)
        switchingData.time = next.time;
    else // Emergency
    {
        switchingData.time = FIRE_TIMER_IMMEDIATELY;   // provoke firing immediately
    }
    // If the time was previously increased, put it back to Normal
    if (current.time != DEFAULT_SWITCHING_TIME)
        current.time = DEFAULT_SWITCHING_TIME;

    current_config_idx = next_idx;

    return switchingData;
}

/*
 * Evaluates if it needs to change configuration
 */
/*
 * if semaphore where it is coming  is ON signal variable to not cahnge on next configurration
 * if not: turn off all the semaphores and turn on a configuraion with that direction
 */
// else does nothing: the Lights timeout does not matter for the  Emergency State
int TrafficControlSystem::EVneedChangeConfiguration ()
{
    tx_cloud::EmergencyContext info = emergencies.front();
    const int EVorigin = info.Origin;

    // Evaluate is that semaphore is already ON
    for (const auto &tsem: configurations[current_config_idx].activeTsem)
    {
        if (EVorigin == tsem->getLocation())
            return -1; // THAT SEMAPHORE IS ALREADY ON
    }

    // If the semaphore where it is coming from is NOT ON, search for a configuration where it is ON
    for (int i = 0; i < configurations.size(); i++)
    {
        for (const auto &tsem : configurations[i].activeTsem)
        {
            if (EVorigin == tsem->getLocation())
            {
                return i;     // It is guaranteed that this will exist
            }
        }
    }
    return -2; // Code will never reach this point
}

TrafficControlSystem::SwitchLightsData TrafficControlSystem::systemWarning()
{
    SwitchLightsData switchingData;

    switchingData.OFF_Crosswalk.clear();
    for (const auto& cw : crosswalks)
        switchingData.OFF_Crosswalk.push_back(cw.get());

    switchingData.OFF_Tsem.clear();
    for (const auto& tsem : TrafficSemVector)
        switchingData.OFF_Tsem.push_back(tsem.get());

    switchingData.time = 5;
    switchingData.ON_Crosswalk.clear();
    switchingData.ON_Tsem.clear();

    return switchingData;
}

void TrafficControlSystem::sendToCloud(CloudSendType message)
{
    cloud.cloudSendQueue.send (std::move(message));
}

void TrafficControlSystem::updateSemaphoresCloud(
    TrafficSemaphore* sem, int light_state)
{
    tx_cloud::TrafficSemaphoreUpdate u{
        .location = sem->getLocation(),
        .status = light_state
    };
    cloud.cloudSendQueue.send(u);
}

void TrafficControlSystem::updateSemaphoresCloud(
    Crosswalk* cross, int light_state)
{
    tx_cloud::PedestrianSemaphoreUpdate p1{
        .location = cross->psem1->getLocation(),
        .status = light_state
    };
    tx_cloud::PedestrianSemaphoreUpdate p2{
        .location = cross->psem2->getLocation(),
        .status = light_state
    };
    cloud.cloudSendQueue.send(p1);
    cloud.cloudSendQueue.send(p2);
}


/*
void TrafficControlSystem::updateCloud(SwitchLightsData& data, bool isYellow)
{
    int off_update = isYellow ? static_cast<int>(Semaphore::TrafficColour::YELLOW) :
                                static_cast<int>(Semaphore::TrafficColour::RED);

    tx_cloud::TrafficSemaphoreUpdate tsem_update;
    tx_cloud::PedestrianSemaphoreUpdate psem_update;

    for (const auto& element : data.OFF_Tsem)
    {
        tsem_update = {.location = element->getLocation(), .status = off_update};
        cloud.cloudSendQueue.send(tsem_update);
    }

    for (const auto& element : data.OFF_Crosswalk)
    {
        psem_update = {.location = element->psem1->getLocation(), .status = 0};
        cloud.cloudSendQueue.send(psem_update);
        psem_update = {.location = element->psem2->getLocation(), .status = 0};
        cloud.cloudSendQueue.send(psem_update);
    }


    if (isYellow)
    {
        for (const auto& element : data.OFF_Tsem)
        {
            tsem_update = {.location = element->getLocation(), .status = off_update};
            cloud.cloudSendQueue.send(tsem_update);
        }
    }
    else
    {
        for (const auto& element : data.ON_Tsem)
        {
            tsem_update = {.location = element->getLocation(), .status = 1};
            cloud.cloudSendQueue.send(tsem_update);
        }

        for (const auto& element : data.ON_Crosswalk)
        {
            psem_update = {.location = element->psem1->getLocation(), .status = 1};
            cloud.cloudSendQueue.send(psem_update);
            psem_update = {.location = element->psem2->getLocation(), .status = 1};
            cloud.cloudSendQueue.send(psem_update);
        }
    }
}
*/

// Finds the next configuration (after the current) where a card was passed, based on the location
// Increases the time there if it hasn't been increased yet
// Only applicable to the current configuration if the PSEM which is supposed to be on hasn't turned on yet
void TrafficControlSystem::searchConfigurationForRFID(int location)
{
    const int n = configurations.size();

    for (int count = 0; count < n; ++count)
    {
        int i = (current_config_idx + count) % n;

        for (const auto& crosswalk : configurations[i].crosswalk)
        {
            if (crosswalk->psem1->getLocation() == location ||
                crosswalk->psem2->getLocation() == location)
            {
                if (configurations[i].time <= DEFAULT_SWITCHING_TIME)
                {
                    configurations[i].time += DEFAULT_SWITCHING_TIME;
                }
                break;
            }
        }
    }
}

void TrafficControlSystem::stopCurrentTime()
{
    currentSwitchingData.time = 0;
}

void TrafficControlSystem::consumer()
{
    Event data = eventQueue.receive();
    TrafficStrategy->controlOperation(this, data);
}

void* TrafficControlSystem::t_tcs(void* arg)
{
    const auto self = static_cast<TrafficControlSystem*>(arg);

    while (!_shutdown_requested.load())
    {
        self->consumer();
    }
    return arg;
}

void* TrafficControlSystem::t_switchLight(void* arg)
{
    auto self = static_cast<TrafficControlSystem*>(arg);

    SwitchLightsData&  switchingData = self->currentSwitchingData;
    while (!_shutdown_requested.load())
    {
        // Wait for switching data to be ready
        switchingData = self->switchLightQueue.receive();

        // Change Semaphores
        std::cerr << "PSEM OFF \n";
        self->stopPedestriansCross(switchingData.OFF_Crosswalk, false);

        std::cerr << "YELLOW \n";
        self->prepareToStopCars(switchingData.OFF_Tsem);

        self->timerSwitchLight.timerRun(YELLOW_DURATION);
        self->timerSwitchLight.timerWait();

        self->notify(nullptr, InternalEvent::YELLOW_TIMEOUT);

        self->stopCarsMove(switchingData.OFF_Tsem);

        self->letPedestriansCross(switchingData.ON_Crosswalk, false);
        self->letCarsMove(switchingData.ON_Tsem);

        std::cerr<<"GREEN: config "<< self->current_config_idx <<"  \n";

        self->timerSwitchLight.timerRun(switchingData.time);
        self->timerSwitchLight.timerWait();

        // Notify the system itself
        self->notify(nullptr, InternalEvent::LIGHTS_TIMEOUT);
        std::cerr<<"RED\n";
    }
    return arg;
}