// Owns and executes a bot's sensing plan: decides where the bot looks next and rebuilds the plan
// that turns its view there.

#ifndef AI_AIDLL_AI_AIBOT_SENSINGAGENT_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_SENSINGAGENT_H

#include <utility>
#include <vector>

#include "../../../../Math/Vec3.h"     // Vec2 and Vec3 live here, and both are typedefs

namespace dice {
namespace hfe {
namespace ai {

class BAPContext;
// the plan's base class. `SensingAgent` holds one `BAPStatement*` and nothing else about the
// statement tree.
class BAPStatement;
class BotMain;
class BotSenses;
class Information;
class ISensingAgentPattern;

class SensingAgent
{
public:
    // The second parameter is in the ABI and is never read.
    SensingAgent(BotMain* owner, unsigned char behaviourCount);
    // Deletes the senses and the context; the plan is not deleted here.
    ~SensingAgent();

    void reset();
    // Called by `BotMain::reset` when a bot loses its active plan - losing a plan releases the
    // bot's aim override.
    void setOverride(bool);
    void deletePlan();
    void clearPlanTargetInformation();
    // Once per tick: picks where to look and rebuilds the plan when it is due.
    void update();
    // Runs the agent's own plan once; a failed or finished plan clears the plan target.
    void executePlan();
    void disallowOverride(bool disallow);
    // [INFERRED] accessor; BotMain::planExecution inlines the load.
    bool hasPlan() const { return m_plan != 0; }
    // [INFERRED] accessors; SensingAgentSoldierPattern::initPlan inlines the loads and the store.
    BAPStatement* getPlan() const { return m_plan; }
    void setPlan(BAPStatement* plan) { m_plan = plan; }
    BAPContext* getBAPContext() const { return m_bapContext; }
    // [INFERRED] names; BAPAMergeSoldierInputs inlines the loads.
    const Vec2& getMoveDirection() const { return m_moveDirection; }
    const Vec3& getMoveTargetPosition() const { return m_moveTargetPosition; }
    unsigned int getMoveTarget() const { return m_moveTarget; }
    // [CHOSEN] setters; BAPAMoveTo::setIsSprinting stores the fields inline.
    void setMoveTargetPosition(const Vec3& position) { m_moveTargetPosition = position; }
    void setMoveTarget(unsigned int object) { m_moveTarget = object; }
    // Recompute the sensing plan on the next update (a behaviour switched the bot's plan).
    void forceUpdate() { m_forceUpdate = true; }
    // The plan statements' override of where to look: a weight, a direction and whether to
    // recompute at once. The bot's current equipment becomes the plan target.
    void insertPlanDirectionInformation(float weight, const Vec3& direction, bool force);
    // The same for a target. Empty in the shipped binary (a bare `ret`).
    void insertPlanTargetInformation(float weight, unsigned int target, bool force);
    // A behaviour's hint of where to look. Empty in the shipped binary (a bare `ret`).
    void insertBehaviourInformation(float weight, const Vec3& position, unsigned char behaviour);

    // The eight quadrants around the bot: 0 N, 1 E, 2 S, 3 W, 4 NE, 5 SE, 6 SW, 7 NW, as unit
    // directions in the XZ plane (x east, y north).
    void getQuadPosition(int quad, Vec2& position) const;
    bool isForward(int quad, const Vec2& direction) const;
    bool isBehind(int quad, const Vec2& direction) const;
    // Whether the bot can turn its view into the quadrant (or, for -1, onto the plan target).
    bool targetFeasible(int quad) const;
    // Whether a direction lies within the current vehicle's camera limits.
    bool withinCamView(const Vec2& direction) const;
    // Whether the camera looks straight into the quadrant (within ten degrees).
    bool quadInFocus(int quad) const;
    // Clears the weight of the quadrants the camera looks into.
    void setQuadSeen();
    // The axis quadrant and the diagonal quadrant a position lies in, seen from the camera.
    // False when the position is on top of the camera.
    bool computeQuadrantsFromPosition(const Vec3& position, int* quads) const;
    void createDirPlan(const Vec3& direction, ISensingAgentPattern* pattern);
    void createNullPlan();
    // [INFERRED] name; BotMain::getSensedObjects inlines the load.
    BotSenses* getBotSenses() const { return m_senses; }

    // Something worth looking at: a weight and where it is.
    struct FocusInformation
    {
        float weight;
        Vec3  position;
    };

    // The quadrant chosen to look into, its weight and the mean position of what is there.
    struct QuadrantInformation
    {
        int   quad;
        float weight;
        Vec3  position;
    };

private:
    typedef std::vector<std::pair<float, std::pair<Vec3, unsigned int> > > QuadrantWeights;

    void updateQuadTimeUrgencies(Vec2& direction, bool behind);
    void calcTargetQuadFromObjects(QuadrantInformation* quadrant, const Vec2& direction);
    void evaluateSensedObjects();
    void evaluateFireObjects();
    void getQuadrantWeights(QuadrantWeights& weights);
    void computeTargetQuadrant(const QuadrantWeights& weights, QuadrantInformation* quadrant);
    void calculateCurrentPeriod(QuadrantInformation* quadrant, Information* equipment,
                                const Vec2& direction);
    void checkForPlanInvalidation(QuadrantInformation* quadrant, bool behind);
    void createPlan(ISensingAgentPattern* pattern, QuadrantInformation* quadrant);
    void clearInformation();

    /*   0 */ std::vector<FocusInformation> m_sensedFocus;   // evaluateSensedObjects
    /*  12 */ std::vector<FocusInformation> m_fireFocus;     // evaluateFireObjects
    /*  24 */ int   m_currentQuad;         // reset: -1; the quadrant the plan looks into
    /*  28 */ float m_currentQuadWeight;   // reset: 0
    /*  32 */ Vec3  m_currentQuadPosition;
    /*  44 */ bool  m_currentQuadSeen;     // reset: false; setQuadSeen saw m_currentQuad
    // the eight-quadrant weight ring: the time each quadrant has gone unwatched, in urgency.
    /*  48 */ std::vector<float> m_quadrantWeights;
    // The handle the plan statements asked to look at (the bot's current equipment when they
    // gave a direction instead).
    /*  60 */ unsigned int m_planTarget;   // clearPlanTargetInformation: 0
    /*  64 */ Vec3  m_planDirection;       // insertPlanDirectionInformation
    /*  76 */ float m_planDirectionWeight; // clearPlanTargetInformation: 0
    // Where the bot wants to go, for BAPAMergeSoldierInputs to turn into inputs: an XZ direction
    // and a target (handle, -1 for none, else the position). [UNRESOLVED] writer: nothing in this
    // class writes them - presumably the movement entries. The update looks along the direction
    // when it does not look around.
    /*  80 */ Vec2  m_moveDirection;       // reset: 0
    /*  88 */ Vec3  m_moveTargetPosition;  // ctor: 0
    /* 100 */ unsigned int m_moveTarget;   // not written by the constructor or reset
    /* 104 */ BotSenses* m_senses;         // new(0x94) - the mechanism this agent steers
    /* 108 */ BotMain*   m_bot;            // the ctor argument
    /* 112 */ double m_lastUpdateTime;     // reset: the game time; update: the last replan
    /* 120 */ bool m_forceUpdate;          // forceUpdate, insertPlanDirectionInformation, enemy fire
    // [INFERRED] name: set with m_forceUpdate on the first enemy fire, cleared by a plain replan.
    /* 121 */ bool m_enemyFireNoticed;
    /* 122 */ bool m_overrideRequested;    // setOverride; update drops it after waitTime
    /* 123 */ char m_pad123;
    /* 124 */ double m_overrideTime;       // setOverride: the game time
    /* 132 */ bool m_overrideDisallowed;   // reset: false; disallowOverride's argument
    // [INFERRED] name: whether the current quadrant was ahead of the bot when the plan was made.
    /* 133 */ bool m_lookingForward;
    /* 134 */ bool m_flag134;              // ctor: false
    /* 135 */ char m_pad135;
    // the plan itself - one owned statement, guarded everywhere it is touched.
    /* 136 */ BAPStatement* m_plan;        // ctor: 0
    // Set by `reset` and `deletePlan`; update builds a new plan while it holds.
    /* 140 */ bool m_planInvalid;
    /* 141 */ char m_pad141[3];
    /* 144 */ BAPContext* m_bapContext;    // new(4) - shared vocabulary with BotMain's own
    // [INFERRED] name: true for a plan into a quadrant, false for one at the plan target.
    /* 148 */ bool m_lookingIntoQuadrant;  // reset: true
    /* 149 */ char m_pad149[3];
    // How long to keep the plan before looking around again; reset draws it in [0.5, 1.0) and
    // calculateCurrentPeriod redraws it at every replan.
    /* 152 */ float m_reactionTime;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SENSINGAGENT_H
