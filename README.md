# CitySimSystem
### Real-Time Deterministic Multi-Agent Simulation (Unreal Engine C++)

A systems-engineering study in Unreal C++: hundreds of agents living a daily lifecycle on one deterministic, framerate-independent clock, built from decoupled subsystems and instrumented so every decision is visible while it runs. The emphasis is the architecture and observability, not shipped gameplay.

---

##  Demo


Demo Video => https://www.youtube.com/watch?v=RLmhAbE9Z78

<img width="2752" height="1534" alt="ArchitectureBreakdown" src="https://github.com/user-attachments/assets/2a223095-7b81-47dd-a2e3-d7dba6a90fa3" />

<img width="1067" height="660" alt="Screenshot 2026-03-25 193737" src="https://github.com/user-attachments/assets/4414b0c4-ba30-465e-9c5b-17e11578ba70" />

<img width="1064" height="646" alt="Screenshot 2026-03-25 190434" src="https://github.com/user-attachments/assets/c21963da-f8fb-48db-9859-34f76099a58e" />

<img width="1066" height="662" alt="Screenshot 2026-03-25 190539" src="https://github.com/user-attachments/assets/857f176d-3232-4615-b97a-ad20457033d5" />

<img width="1072" height="658" alt="Screenshot 2026-03-25 190636" src="https://github.com/user-attachments/assets/f094b2b1-544e-4a7d-9404-9ceb8c6ddf21" />

---

##  What This Is

This is an engineering project first and a game second. The value is in how it is built, not in claiming perfect behaviour:

- A **fixed-timestep core** advances the whole city in exact 0.1s steps, so the logic runs the same number of steps at 30 FPS or 300.
- **Data-oriented agents**: every citizen is a plain struct in one contiguous array, mutated in place, with disposable visual proxies drawing on top.
- **Decoupled World Subsystems**: no monolithic GameMode. Needs, AI, scheduling, pathfinding, interaction and transit are separate systems, ticked in a fixed order.
- **Built to be observed**: agent state, goals, routes and per-step cost are all exposed live.

Agents follow a full daily lifecycle (home → work → leisure). The behaviour layer is a work in progress; the substrate underneath is the point.

---

##  Core Systems

- **Fixed-Timestep Simulation Core**  
  One 10 Hz accumulator loop drives every subsystem in a fixed order, with a bounded catch-up so a hitch cannot lock it up. Seeded random streams mean the same seed rebuilds the same city and drives the same schedule.

- **Agent Decision Layer**  
  A daily schedule sets each agent's goal; in free time a lowest-need rule can override it, and the chosen goal resolves to a building scored by distance and occupancy so crowds spread instead of dogpiling. A commitment lock keeps decisions stable rather than flip-flopping.

- **Interaction / Smart Objects**  
  Buildings register as capacity-gated smart objects. An agent reserves a slot before it travels, and because the sim ticks single-threaded the check-and-claim is safe without a lock. A miss re-routes to the next-best venue.

- **Transit & Routing**  
  Pedestrians route with a custom A* over a coarse road graph (not the engine navmesh), each pushed to one of two lanes by ID so foot traffic does not walk single-file down the road centre. Vehicles route over the same road edges with a breadth-first search, and brake or stop for pedestrians in front of them.

- **Real-Time Control**  
  Time scaling, weather, and three camera modes, all adjustable while it runs.

---

##  Debug & Observability

The point of the architecture is that internal state is visible in real time:

- Agent state, current goal, and decision reason  
- A* routes and targets over the road graph  
- Population, the world clock, and per-step simulation cost  

### Debug Commands


Sim_DebugStats
Sim_DebugAgents
Sim_DebugPaths
Sim_DebugVehicles
Sim_DebugTransit
Sim_DebugSocial
Sim_DebugZones
Sim_DebugDensity
Sim_DebugSpatialGrid


---

##  Architecture

The simulation is built from decoupled subsystems, ticked in a fixed order by one orchestrator:

- `UWorldSimulationSubsystem` → master clock & fixed-step orchestration  
- `UNeedsSubsystem` → agent data & need evaluation  
- `USimAISubsystem` → decision making & movement logic  
- `UInteractionSubsystem` → smart-object management & reservation  
- `ULifeSchedulerSubsystem` → the daily lifecycle  

### Pipeline


Decision → Target → Path → Movement


---

##  Controls

**Camera**
- `1` → Eagle Eye  
- `2` → Free  
- `3` → Follow  

**Simulation**
- `7` → 1x speed  
- `8` → 5x speed  
- `9` → 20x speed  
- `R` → Toggle Rain
- Arrow Left and Right Keys → Switch Between Previous and Next Agent 

**Movement**
- `WASDQE` → Move  
- Mouse → Look  

---

##  Performance & Scaling

- Runs **hundreds of agents in real time** at **under 1 ms per simulation step** (~0.8 ms at 300 agents).  
- Fixed-order subsystem ticks; pooled A* buffers to avoid per-query allocations.  
- Pathfinding is structured as a deferred request/resolve queue (built to move off-thread) and currently resolves inline.  

> Current limitation: the visual layer uses one `ACharacter` proxy per agent, which caps the practical count around a few hundred. The simulation layer already handles more.  
> Next: MassEntity / instancing for 10k+ agents.

---

##  Contact

studioslucidframes@gmail.com  
Mumbai, India
